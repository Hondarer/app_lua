# ビルド済みライブラリの検索パス
LIBSDIR += \
	$(MYAPP_DIR)/prod/lib

# ライブラリの指定
#
# 本テストは Lua ソース自体のカバレッジ計測ではなく、
# app/lua が提供する API (liblua.a) の独自動作確認を目的とする。
# そのため TEST_SRCS は使用せず、ビルド済みライブラリへの通常のリンクで検証する。
LIBS += lua m

ifdef PLATFORM_LINUX
    LIBS += dl
endif
