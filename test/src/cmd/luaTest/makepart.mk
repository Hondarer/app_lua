# ビルド済みライブラリの検索パス
LIBSDIR += \
	$(MYAPP_DIR)/prod/lib

# ライブラリの指定
#
# 本テストは Lua ソース自体のカバレッジ計測ではなく、
# app/lua が提供する API (liblua.a) の独自動作確認を目的とする。
# そのため TEST_SRCS は使用せず、ビルド済みライブラリへの通常のリンクで検証する。
LIBS += lua

ifdef PLATFORM_LINUX
    # Lua の数学関数に libm、動的ライブラリ読み込み実装に libdl が必要
    LIBS += m dl
endif
