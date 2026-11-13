#include <lua.h>

const char *namespace_test_cpp_caller_file()
{
    return __FILE__;
}
/* 呼び出し行の照合に使うため、次の 2 定義を隣接させます。 */
// clang-format off
int namespace_test_cpp_caller_line() { return __LINE__ + 1; }
int namespace_test_cpp_direct_call(lua_State *state) { return lua_app_lua_gettop(state); }
// clang-format on
