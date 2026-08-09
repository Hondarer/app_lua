# shared-only への移行前に生成された静的ライブラリを残さない。
# フック検出は makelocal.mk のテキスト走査のため、ターゲット定義は
# PLATFORM 条件の外に置き、レシピ内で分岐する。
.PHONY: pre-build pre-clean

pre-build:
ifdef PLATFORM_LINUX
	$(RM) "$(OUTPUT_DIR)/liblua.a" "$(OUTPUT_DIR)/liblua.a.warn"
endif

pre-clean:
ifdef PLATFORM_LINUX
	$(RM) "$(OUTPUT_DIR)/liblua.a" "$(OUTPUT_DIR)/liblua.a.warn"
endif
