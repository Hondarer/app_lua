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
