ifdef PLATFORM_WINDOWS
    # Lua の DLL import ではなくモックの実シンボルを定義する。
    DEFINES += LUA_CORE
endif
