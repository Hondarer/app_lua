# 公式インタープリタ (lua.c) から lua コマンドをビルドする。
#
# lua.c は app/lua/packages 配下の tar.gz から機械的に展開される生成物であり、
# 手動改変しない (再展開で上書きされる)。行編集オプション (LUA_USE_READLINE) は
# 定義せず、readline への外部依存を避けた最小構成でビルドする。

LIBSDIR += \
	$(MYAPP_DIR)/prod/lib

LIBS += lua

ifdef PLATFORM_LINUX
    # Lua の数学関数に libm、loadlib.c の動的ライブラリ読み込み実装に libdl が必要
    LIBS += m dl
endif
