/* 可変長 API は通常の関数 API 表と分けて管理する。 */
#ifndef MOCK_VARIADIC
    #error MOCK_VARIADIC must be defined
#endif

MOCK_VARIADIC(const char *, lua_pushfstring, lua_app_lua_pushfstring, (lua_State *state, const char *format), (state, format), (const char *caller_file, int caller_line, const char *caller_function, lua_State *state, const char *format, va_list args), (caller_file, caller_line, caller_function, state, format, args))
MOCK_VARIADIC(int, lua_gc, lua_app_lua_gc, (lua_State *state, int operation), (state, operation), (const char *caller_file, int caller_line, const char *caller_function, lua_State *state, int operation, va_list args), (caller_file, caller_line, caller_function, state, operation, args))
MOCK_VARIADIC(int, luaL_error, lua_app_luaL_error, (lua_State *state, const char *format), (state, format), (const char *caller_file, int caller_line, const char *caller_function, lua_State *state, const char *format, va_list args), (caller_file, caller_line, caller_function, state, format, args))
