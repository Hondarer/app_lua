#ifndef MOCK_OVERRIDE_LUA_LUALIB_H
#define MOCK_OVERRIDE_LUA_LUALIB_H

#if !defined(MOCK_LUA_ENABLE_CALLER) || defined(MOCK_LUA_IN_OVERRIDE_REAL_HEADERS)
    #if defined(_WIN32)
        /* see: https://learn.microsoft.com/en-us/cpp/preprocessor/warning */
        #pragma warning(push, 0)
        #include "../../prod/include/lualib.h"
        #pragma warning(pop)
    #else
        #include_next <lualib.h>
    #endif
#else
    #define MOCK_LUA_IN_OVERRIDE_REAL_HEADERS
    #ifdef __cplusplus
extern "C"
{
    #endif
    #if defined(_WIN32)
        /* see: https://learn.microsoft.com/en-us/cpp/preprocessor/warning */
        #pragma warning(push, 0)
        #include "../../prod/include/lua.h"
        #include "../../prod/include/lauxlib.h"
        #include "../../prod/include/lualib.h"
        #pragma warning(pop)
    #else
        #include_next <lua.h>
        #include_next <lauxlib.h>
        #include_next <lualib.h>
    #endif
    #ifdef __cplusplus
}
    #endif
    #undef MOCK_LUA_IN_OVERRIDE_REAL_HEADERS
    #define MOCK_LUA_INJECT_CALLER
    #include <mock_lua_overrides.h>
    #undef MOCK_LUA_INJECT_CALLER
#endif

#endif /* MOCK_OVERRIDE_LUA_LUALIB_H */
