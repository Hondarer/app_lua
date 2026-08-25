# Lua ソース (コア VM + 標準ライブラリ) を取り込む。
#
# 本ディレクトリの .c/.h ファイルおよび ../../include/ 配下のヘッダーは
# app/lua/packages 配下の tar.gz から機械的に展開される生成物であり、
# 手動改変しない (再展開で上書きされる)。そのため、本リポジトリの
# コーディング規範 (goto・三項演算子の不使用、新規ファイルへの
# clang-format 適用) は Lua 本体には適用しない。
# (このディレクトリの makefile/makepart.mk 自体は手書きファイルであり、
#  通常どおり規範・clang-format の対象とする)

# 外来ヘッダーの警告は SYSTEM_INCDIR で利用側から分離する。
# upstream の一次ソース自体に残る警告だけを、このリーフで抑制する。
ifdef PLATFORM_LINUX
    CFLAGS   += -Wno-padded -Wno-cast-qual -Wno-switch-default -Wno-switch-enum -Wno-format-nonliteral -Wno-conversion -Wno-sign-conversion
    CXXFLAGS += -Wno-padded -Wno-cast-qual -Wno-switch-default -Wno-switch-enum -Wno-format-nonliteral -Wno-conversion -Wno-sign-conversion
endif

ifdef PLATFORM_WINDOWS
    CFLAGS   += /wd4310 /wd4324 /wd4701 /wd4702 /wd4709
    CXXFLAGS += /wd4310 /wd4324 /wd4701 /wd4702 /wd4709
endif

ifdef PLATFORM_LINUX
    # 公開 API 以外を hidden とし、LUA_API のシンボルだけを公開する。
    CFLAGS   += -fvisibility=hidden
    CXXFLAGS += -fvisibility=hidden
endif

# 静的ライブラリは生成せず、Linux では .so、Windows では .dll を生成する。
LIB_TYPE = shared
