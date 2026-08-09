#ifndef MOCK_LUA_H
#define MOCK_LUA_H

#include <lua.hpp>
#include <testfw.h>

#include <cstdarg>

inline constexpr char kLibLuaName[] = "liblua" TESTFW_SHARED_LIBRARY_EXTENSION;

#define MOCK_LUA_RET(return_type, name, parameters, arguments, matchers) \
    extern return_type delegate_real_##name parameters;
#define MOCK_LUA_VOID(return_type, name, parameters, arguments, matchers) \
    extern return_type delegate_real_##name parameters;
#include <mock_lua_api_table.h>
#undef MOCK_LUA_VOID
#undef MOCK_LUA_RET

extern const char *delegate_real_lua_pushfstring(lua_State *state, const char *format, va_list args);
extern int delegate_real_lua_gc(lua_State *state, int operation, va_list args);
extern int delegate_real_luaL_error(lua_State *state, const char *format, va_list args);

class Mock_lua
{
  public:
#define MOCK_LUA_RET(return_type, name, parameters, arguments, matchers)  MOCK_METHOD(return_type, name, parameters);
#define MOCK_LUA_VOID(return_type, name, parameters, arguments, matchers) MOCK_METHOD(return_type, name, parameters);
#include <mock_lua_api_table.h>
#undef MOCK_LUA_VOID
#undef MOCK_LUA_RET

    MOCK_METHOD(const char *, lua_pushfstring, (lua_State *, const char *, va_list));
    MOCK_METHOD(int, lua_gc, (lua_State *, int, va_list));
    MOCK_METHOD(int, luaL_error, (lua_State *, const char *, va_list));

    Mock_lua();
    ~Mock_lua();
};

extern Mock_lua *_mock_lua;

#endif /* MOCK_LUA_H */
