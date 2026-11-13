#include <lua.h>

const char *namespace_test_caller_file(void);
int namespace_test_caller_line(void);
int namespace_test_direct_call(lua_State *state);
int namespace_test_pointer_call(lua_State *state);

const char *namespace_test_caller_file(void)
{
    return __FILE__;
}
/* 呼び出し行の照合に使うため、次の 2 定義を隣接させます。 */
// clang-format off
int namespace_test_caller_line(void) { return __LINE__ + 1; }
int namespace_test_direct_call(lua_State *state) { return lua_app_lua_gettop(state); }
// clang-format on
int namespace_test_pointer_call(lua_State *state)
{
    int (*api)(lua_State *) = &lua_app_lua_gettop;
    return api(state);
}

int namespace_test_variadic_line(void);
int namespace_test_variadic_call(void);
/* 呼び出し行の照合に使うため、次の 2 定義を隣接させます。 */
// clang-format off
int namespace_test_variadic_line(void) { return __LINE__ + 1; }
int namespace_test_variadic_call(void) { return lua_app_lua_gc(NULL, LUA_GCCOUNT); }
// clang-format on
