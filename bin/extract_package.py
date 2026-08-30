#!/usr/bin/env python3
"""app/lua/bin/extract_package.py

packages/ 配下の Lua ソース アーカイブ (tar.gz) を prod/include,
prod/libsrc/lua, prod/src/cmd/lua へ展開する。外部ツール (tar 等) に
依存せず、標準ライブラリ tarfile のみを使用する。

展開後、patches/ 配下の unified diff (framework/makefw/bin/apply_patches.py)
を順に適用する。tar の内容は加工せずそのまま書き出し、Lua 本体への
改変はすべてパッチ側で行う。
"""

import argparse
import errno
import os
import re
import stat
import sys
import tarfile
import tempfile
import time
from pathlib import Path

sys.stdout.reconfigure(encoding="utf-8")
sys.stderr.reconfigure(encoding="utf-8")

PACKAGE_NAME_PATTERN = re.compile(r"^lua-.*\.tar\.gz$", re.IGNORECASE)
VERSION_PATTERN = re.compile(r"^lua-(\d+)\.(\d+)\.(\d+)\.tar\.gz$", re.IGNORECASE)

# 公開ヘッダー (組み込み利用者向け。prod/include 直下へ展開する)
PUBLIC_HEADERS = ["lua.h", "luaconf.h", "lualib.h", "lauxlib.h", "lua.hpp"]

# コア VM の実装 (src/Makefile の CORE_O に対応)
CORE_SRCS = [
    "lapi.c", "lcode.c", "lctype.c", "ldebug.c", "ldo.c", "ldump.c", "lfunc.c", "lgc.c",
    "llex.c", "lmem.c", "lobject.c", "lopcodes.c", "lparser.c", "lstate.c", "lstring.c",
    "ltable.c", "ltm.c", "lundump.c", "lvm.c", "lzio.c",
]

# 標準ライブラリの実装 (src/Makefile の LIB_O に対応)
LIB_SRCS = [
    "lauxlib.c", "lbaselib.c", "lcorolib.c", "ldblib.c", "liolib.c", "lmathlib.c",
    "loadlib.c", "loslib.c", "lstrlib.c", "ltablib.c", "lutf8lib.c", "linit.c",
]

# 内部ヘッダー (src/ 配下の .c 間だけで参照する非公開ヘッダー)
INTERNAL_HEADERS = [
    "lapi.h", "lcode.h", "lctype.h", "ldebug.h", "ldo.h", "lfunc.h", "lgc.h", "ljumptab.h",
    "llex.h", "llimits.h", "lmem.h", "lobject.h", "lopcodes.h", "lopnames.h", "lparser.h",
    "lprefix.h", "lstate.h", "lstring.h", "ltable.h", "ltm.h", "lundump.h", "lvm.h",
    "lzio.h",
]

# 公式インタープリター実行ファイル (lua.c と対称に luac.c は取り込まない)
MAIN_SRC = "lua.c"

# lprefix.h (機能検査マクロ定義) と llimits.h (内部の型/制限マクロ定義) は、
# コア/標準ライブラリ側の .c だけでなく lua.c からも同一ディレクトリ相対で
# #include される。INCDIR には include/・include_internal/ のみを追加する
# 方針のため、prod/libsrc/lua を lua.c 側の追加検索パスにはできない。そのため
# これらのヘッダーだけは prod/src/cmd/lua/ にも複製し、同一ディレクトリ解決で
# コンパイルできるようにする。
SHARED_INTERNAL_HEADERS = ["lprefix.h", "llimits.h"]

# 展開対象: tar 内のファイル名 -> 展開先のリスト (プレースホルダーは app_dir からの相対パス)。
# 通常は 1 ファイル 1 展開先だが、lprefix.h だけは 2 箇所へ複製する。
EXTRACT_TARGETS = {}
EXTRACT_TARGETS.update({name: [("prod", "include", name)] for name in PUBLIC_HEADERS})
EXTRACT_TARGETS.update(
    {name: [("prod", "libsrc", "lua", name)] for name in CORE_SRCS + LIB_SRCS + INTERNAL_HEADERS}
)
for _header in SHARED_INTERNAL_HEADERS:
    EXTRACT_TARGETS[_header].append(("prod", "src", "cmd", "lua", _header))
EXTRACT_TARGETS[MAIN_SRC] = [("prod", "src", "cmd", "lua", MAIN_SRC)]

# 展開後、パッチ適用前に代表ファイルを最後に置換する順序に使う。
MARKER_SOURCE = "lapi.c"

# 再展開要否の判定に使うスタンプ ファイル。
STAMP_FILENAME = "make_extract.stamp"

# 生成物を除外するための .gitignore を配置するディレクトリと、その内容。
#
# framework/makefw/makefiles/makelibsrc_c_cpp.mk / makesrc_c_cpp.mk は、
# TEST_SRCS/ADD_SRCS を使わないビルド リーフ ディレクトリでは、
# `make clean` のたびに無条件で .gitignore を削除する (app/cjson で判明した
# 挙動と同じ)。prod/libsrc/lua、prod/src/cmd/lua はいずれも該当するため、
# ここで毎回 (再展開の有無に関わらず) .gitignore を再生成し、`make clean` 後も
# 次回 make で必ず復元されるようにする。
GITIGNORE_TARGETS = {
    ("prod", "libsrc", "lua"): CORE_SRCS + LIB_SRCS + INTERNAL_HEADERS,
    ("prod", "src", "cmd", "lua"): [MAIN_SRC] + SHARED_INTERNAL_HEADERS,
}

GITIGNORE_HEADER = "# app/lua/packages 配下の tar.gz から展開される生成物。手動改変しないため Git 管理対象外とする。\n"


def _same_content(path, data):
    """path の内容が data と一致すれば True。読めない場合は False。"""
    try:
        if isinstance(data, str):
            with open(path, "r", encoding="utf-8", newline="") as f:
                return f.read() == data
        with open(path, "rb") as f:
            return f.read() == data
    except OSError:
        return False


def _is_retryable_replace_error(exc):
    """並列プロセスによる Windows の replace 失敗を再試行対象とみなす。"""
    if isinstance(exc, PermissionError):
        return True
    if isinstance(exc, OSError):
        winerror = getattr(exc, "winerror", None)
        # 5: ACCESS_DENIED, 32: SHARING_VIOLATION
        if winerror in (5, 32):
            return True
        if exc.errno in (errno.EACCES, errno.EPERM):
            return True
    return False


def atomic_replace(path, data, retries=10, base_delay=0.05):
    """同じディレクトリの一意な一時ファイルを使ってファイルを置換する。

    内容が既に同一なら何もしない。Windows の並列 make で複数プロセスが
    同一パスへ os.replace する際の PermissionError (WinError 5) 等には
    再試行する。再試行中に他プロセスが正しい内容を書いた場合は成功とみなす。
    """
    if _same_content(path, data):
        return

    dir_path = os.path.dirname(path) or "."
    prefix = f".{os.path.basename(path)}."
    try:
        file_mode = stat.S_IMODE(os.stat(path).st_mode)
    except FileNotFoundError:
        current_umask = os.umask(0)
        os.umask(current_umask)
        file_mode = 0o666 & ~current_umask

    last_err = None
    for attempt in range(retries):
        if attempt > 0 and _same_content(path, data):
            return

        tmp_path = None
        try:
            if isinstance(data, str):
                with tempfile.NamedTemporaryFile(
                    mode="w",
                    encoding="utf-8",
                    newline="",
                    dir=dir_path,
                    prefix=prefix,
                    suffix=".tmp",
                    delete=False,
                ) as f:
                    tmp_path = f.name
                    f.write(data)
            else:
                with tempfile.NamedTemporaryFile(
                    mode="wb",
                    dir=dir_path,
                    prefix=prefix,
                    suffix=".tmp",
                    delete=False,
                ) as f:
                    tmp_path = f.name
                    f.write(data)
            os.chmod(tmp_path, file_mode)
            os.replace(tmp_path, path)
            return
        except OSError as e:
            if not _is_retryable_replace_error(e):
                raise
            last_err = e
            time.sleep(base_delay * (2 ** min(attempt, 4)))
        finally:
            if tmp_path is not None:
                try:
                    os.unlink(tmp_path)
                except FileNotFoundError:
                    pass

    if _same_content(path, data):
        return
    raise last_err


def iter_target_paths(app_dir):
    for rel_parts_list in EXTRACT_TARGETS.values():
        for rel_parts in rel_parts_list:
            yield os.path.join(app_dir, *rel_parts)


def ensure_gitignore(app_dir):
    for rel_parts, names in GITIGNORE_TARGETS.items():
        dir_path = os.path.join(app_dir, *rel_parts)
        os.makedirs(dir_path, exist_ok=True)
        gitignore_path = os.path.join(dir_path, ".gitignore")
        content = GITIGNORE_HEADER + "".join(f"/{name}\n" for name in names)
        atomic_replace(gitignore_path, content)


def find_candidates(packages_dir):
    if not os.path.isdir(packages_dir):
        return []
    return sorted(f for f in os.listdir(packages_dir) if PACKAGE_NAME_PATTERN.match(f))


def parse_version(filename):
    m = VERSION_PATTERN.match(filename)
    if m is None:
        return None
    return tuple(int(part) for part in m.groups())


def select_package(packages_dir, candidates):
    """複数候補がある場合、ファイル名のバージョン番号が最も新しいものを採用する。
    バージョン番号が抽出できないファイルが混在する場合は、mtime が最も新しい
    ものにフォールバックする。"""
    if len(candidates) == 1:
        return candidates[0], []

    versions = {name: parse_version(name) for name in candidates}
    if all(v is not None for v in versions.values()):
        selected = max(candidates, key=lambda name: versions[name])
    else:
        selected = max(
            candidates,
            key=lambda name: os.path.getmtime(os.path.join(packages_dir, name)),
        )
    rejected = [name for name in candidates if name != selected]
    return selected, rejected


def print_missing_package_guide(packages_dir):
    lines = [
        "",
        "ERROR: Lua のソース アーカイブ (tar.gz) が app/lua/packages に見つかりません。",
        "",
        f"  配置先: {packages_dir}",
        "  ファイル名の例: lua-5.5.0.tar.gz (バージョン番号をファイル名に含めること)",
        "",
        "  取得方法 (curl での取得例):",
        "    curl -L -o app/lua/packages/lua-5.5.0.tar.gz \\",
        "      https://www.lua.org/ftp/lua-5.5.0.tar.gz",
        "",
        "  最新版は https://www.lua.org/download.html でも確認できます。",
        "  取得後、このディレクトリには常に 1 個の tar.gz のみを配置してください。",
        "  バージョン更新時は、古いアーカイブを新しいものに置き換えてください。",
        "",
    ]
    print("\n".join(lines), file=sys.stderr)


def print_multiple_package_warning(selected, rejected):
    lines = [
        "",
        "WARNING: app/lua/packages に複数の Lua アーカイブが見つかりました。",
        f"  採用: {selected} (バージョンが最も新しいと判断)",
    ]
    lines += [f"  未採用: {name}" for name in rejected]
    lines += [
        "  単一ファイル運用のため、未採用のアーカイブは削除してください。",
        "",
    ]
    print("\n".join(lines), file=sys.stderr)


def find_member(names, filename):
    """tar 内から <トップディレクトリ>/src/filename に一致するメンバー名を返す。"""
    matches = [n for n in names if n.endswith(f"/src/{filename}") and n.count("/") == 2]
    return matches[0] if matches else None


def stamp_path(app_dir):
    return os.path.join(app_dir, STAMP_FILENAME)


def compute_stamp_fields(tar_path, selected_name, patches_dir, apply_patches_mod):
    """スタンプへ書き出す項目を辞書で返す。tar.gz の stat とパッチ系列の
    digest のみを使い、展開先ファイルの内容はハッシュしない
    (makepart.mk から 1 ビルドで何十回も起動されるため軽量さを優先する)。
    """
    st = os.stat(tar_path)
    return {
        "package": selected_name,
        "tar_mtime": repr(st.st_mtime),
        "tar_size": str(st.st_size),
        "patches_digest": apply_patches_mod.series_digest(Path(patches_dir)),
    }


def read_stamp(path):
    """スタンプ ファイルを {キー: 値} で返す。読めない場合は None。"""
    try:
        with open(path, "r", encoding="utf-8") as f:
            lines = f.read().splitlines()
    except OSError:
        return None

    fields = {}
    for line in lines:
        if not line or "=" not in line:
            return None
        key, _, value = line.partition("=")
        fields[key] = value
    return fields


def write_stamp(path, fields):
    content = "".join(f"{key}={value}\n" for key, value in fields.items())
    atomic_replace(path, content)


def needs_extraction(tar_path, app_dir, selected_name, patches_dir, apply_patches_mod):
    if any(not os.path.isfile(path) for path in iter_target_paths(app_dir)):
        return True

    current = read_stamp(stamp_path(app_dir))
    if current is None:
        return True

    expected = compute_stamp_fields(tar_path, selected_name, patches_dir, apply_patches_mod)
    return current != expected


def extract(tar_path, app_dir):
    """tar の内容を加工せず、EXTRACT_TARGETS の展開先へそのまま書き出す。"""
    dest_paths = {}
    for src_name, rel_parts_list in EXTRACT_TARGETS.items():
        paths = []
        for rel_parts in rel_parts_list:
            dest_path = os.path.join(app_dir, *rel_parts)
            os.makedirs(os.path.dirname(dest_path), exist_ok=True)
            paths.append(dest_path)
        dest_paths[src_name] = paths

    with tarfile.open(tar_path, "r:gz") as tf:
        names = tf.getnames()
        # 代表ファイルを最後に置換し、全出力の準備前に別プロセスが
        # 展開完了と判定しないようにする。
        ordered_sources = [name for name in dest_paths if name != MARKER_SOURCE]
        ordered_sources.append(MARKER_SOURCE)
        for src_name in ordered_sources:
            paths = dest_paths[src_name]
            member = find_member(names, src_name)
            if member is None:
                print(f"ERROR: tar 内に src/{src_name} が見つかりません: {tar_path}", file=sys.stderr)
                return False
            extracted = tf.extractfile(member)
            if extracted is None:
                print(f"ERROR: tar 内のメンバーを読み取れません: {member}", file=sys.stderr)
                return False
            data = extracted.read()
            for dest_path in paths:
                atomic_replace(dest_path, data)

    # パッチ適用がこの後ファイルを書き換え、mtime は適用時刻になる。
    # これはパッチ変更時に make がソースの更新を検知するために必要な
    # 正しい挙動であり、パッチ適用後に mtime を tar.gz の値へ戻さないこと。
    tar_mtime = os.path.getmtime(tar_path)
    for paths in dest_paths.values():
        for dest_path in paths:
            os.utime(dest_path, (tar_mtime, tar_mtime))
    return True


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--app-dir", required=True)
    parser.add_argument(
        "--makefw-home",
        required=True,
        help="framework/makefw のパス。<makefw-home>/bin を sys.path へ加えて "
        "apply_patches を import するために使う。",
    )
    args = parser.parse_args()

    sys.path.insert(0, os.path.join(args.makefw_home, "bin"))
    import apply_patches  # noqa: E402  (sys.path 設定後に import する)

    packages_dir = os.path.join(args.app_dir, "packages")
    patches_dir = os.path.join(args.app_dir, "patches")
    candidates = find_candidates(packages_dir)

    if not candidates:
        print_missing_package_guide(packages_dir)
        return 1

    selected, rejected = select_package(packages_dir, candidates)
    if rejected:
        print_multiple_package_warning(selected, rejected)

    tar_path = os.path.join(packages_dir, selected)

    ensure_gitignore(args.app_dir)

    if not needs_extraction(tar_path, args.app_dir, selected, patches_dir, apply_patches):
        return 0

    stamp_file = stamp_path(args.app_dir)
    try:
        os.remove(stamp_file)
    except FileNotFoundError:
        pass

    print(f"INFO: Lua パッケージを展開しています: {selected}", file=sys.stderr)
    if not extract(tar_path, args.app_dir):
        return 2

    try:
        apply_patches.apply_series(Path(patches_dir), Path(args.app_dir))
    except apply_patches.PatchError as exc:
        print(f"ERROR: Lua パッチの適用に失敗しました: {exc}", file=sys.stderr)
        return 3

    write_stamp(
        stamp_file,
        compute_stamp_fields(tar_path, selected, patches_dir, apply_patches),
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
