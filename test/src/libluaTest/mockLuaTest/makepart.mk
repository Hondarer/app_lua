ifdef PLATFORM_WINDOWS
    # Lua の DLL import ではなく mock_lua が提供する実シンボルを参照する。
    DEFINES += LUA_CORE
endif

LIBS += mock_lua
