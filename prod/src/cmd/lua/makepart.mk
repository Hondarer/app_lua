# 公式インタープリター (lua.c) から lua コマンドをビルドする。
#
# lua.c は app/lua/packages 配下の tar.gz から機械的に展開される生成物であり、
# 手動改変しない (再展開で上書きされる)。行編集オプション (LUA_USE_READLINE) は
# 定義せず、readline への外部依存を避けた最小構成でビルドする。

# upstream の一次ソース自体に残る警告だけを、このリーフで抑制する。
ifdef PLATFORM_LINUX
    CFLAGS   += -Wno-padded -Wno-cast-qual -Wno-switch-default -Wno-switch-enum -Wno-format-nonliteral -Wno-conversion -Wno-sign-conversion
    CXXFLAGS += -Wno-padded -Wno-cast-qual -Wno-switch-default -Wno-switch-enum -Wno-format-nonliteral -Wno-conversion -Wno-sign-conversion
endif

ifdef PLATFORM_WINDOWS
    CFLAGS   += /wd4310 /wd4324 /wd4701 /wd4702 /wd4709
    CXXFLAGS += /wd4310 /wd4324 /wd4701 /wd4702 /wd4709
endif

LIBS += lua

ifdef PLATFORM_LINUX
    # Lua の数学関数に libm、loadlib.c の動的ライブラリ読み込み実装に libdl が必要
    LIBS += m dl
endif
