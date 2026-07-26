# app/lua/makepart.mk
# Lua ソース パッケージの展開を、app/lua 配下のどのディレクトリで make を
# 起動しても最初に保証するためのフック。
#
# 注意: TEST_SRCS / SRCS_C の $(wildcard) 判定は Makefile の読み込み時に
# 即時評価されるため、対象ディレクトリの pre-build (ターゲット実行フェーズ)
# では展開が間に合わない。そのため、より上位の (全階層に継承される)
# makepart.mk で $(shell) を用いて「読み込み時」に展開を完了させる。
# see: framework/makefw/docs/makeparts.md
# (app/cjson/makepart.mk、app/sqlite/makepart.mk と同じ構造)

ifndef MAKEFW_SYNC_EVAL
    _LUA_EXTRACT_STATUS := $(shell python3 "$(MYAPP_DIR)/bin/extract_package.py" --app-dir "$(MYAPP_DIR)" >&2; echo $$?)
    ifneq ($(_LUA_EXTRACT_STATUS),0)
        $(error Lua パッケージの準備に失敗しました。上記のメッセージに従って app/lua/packages にアーカイブを配置してください)
    endif
endif

# Lua 公式ビルド (src/Makefile の linux ターゲット) に合わせて LUA_USE_LINUX を
# 定義する。動的ライブラリ読み込み (require) や POSIX 関連の実装が有効になる。
# readline は取り込まない方針 (README 参照) のため LUA_USE_READLINE は定義しない。
ifdef PLATFORM_LINUX
    CFLAGS   += -DLUA_USE_LINUX
    CXXFLAGS += -DLUA_USE_LINUX
endif

# 警告抑制 (app/makepart.mk の GCC_WARN_BASE) について:
#
# Lua のコア VM・標準ライブラリ・lua.c は upstream (外部) 側の設計であり、
# 本リポジトリ側ではソースを改変しない方針のためレイアウトや実装スタイルを
# 変更できない。本リポジトリは警告抑制フラグ (-Wno-*) を原則使わない方針だが
# (see: framework/makefw/docs/gcc-warning-guide/README.md)、cJSON/sqlite と
# 同様に今回はソース非改変を優先し、Lua 本体に起因する以下の警告に限り
# app/lua 配下のコンパイル オプションで例外的に抑制する (ユーザー承認済み)。
#
# - -Wpadded: 構造体パディング (lua.h の公開 API 構造体。ヘッダー経由で
#   本リポジトリ側のコードにも波及するため、app/lua 全体で抑制する)
# - -Wcast-qual: const 修飾子を外す明示キャスト (内部実装)
# - -Wswitch-default: switch 文の default 分岐省略
# - -Wswitch-enum: VM オペコード switch (lopcodes.c 由来の全列挙値) で
#   一部の列挙値のみ明示的に扱う設計になっているため大量に発生する
# - -Wformat-nonliteral: printf 系関数へ非リテラルの書式文字列を渡す実装
#   (loslib.c/lstrlib.c の言語機能実装上の必然)
#
# app/makepart.mk (親階層) で CFLAGS/CXXFLAGS に GCC_WARN_BASE が設定された後、
# makepart.mk は親から子の順に評価されるため、ここで追記する -Wno-* は
# 常に元の -W* より後方に置かれ、GCC 上で有効に上書きできる。
# MSVC は -W 系オプションを解釈できないため Linux 限定で追記する。
ifdef PLATFORM_LINUX
    CFLAGS   += -Wno-padded -Wno-cast-qual -Wno-switch-default -Wno-switch-enum -Wno-format-nonliteral
    CXXFLAGS += -Wno-padded -Wno-cast-qual -Wno-switch-default -Wno-switch-enum -Wno-format-nonliteral
endif
