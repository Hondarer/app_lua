ifdef PLATFORM_WINDOWS
    # Lua の DLL import ではなく mock_lua が提供する実シンボルを参照する。
    DEFINES += LUA_CORE
endif

LIBS += mock_lua
DEFINES += MOCK_LUA_ENABLE_CALLER
TEST_SRCS += $(MYAPP_DIR)/test/src/libluaTest/mockLuaTest/caller_info.c \
    $(MYAPP_DIR)/test/src/libluaTest/mockLuaTest/caller_info_cpp.cc
