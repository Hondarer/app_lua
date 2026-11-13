#ifndef MOCK_LUA_H
#define MOCK_LUA_H

#include <lua.hpp>
#include <testfw.h>
#include <mock_lua_overrides.h>

#include <cstdarg>

inline constexpr char kLibLuaName[] = "liblua_app" TESTFW_SHARED_LIBRARY_EXTENSION;

#define MOCK_LUA_RET(return_type, original_name, api_name, parameters, arguments, mock_parameters, mock_arguments, matchers) \
    extern return_type delegate_real_##api_name mock_parameters;
#define MOCK_LUA_VOID(return_type, original_name, api_name, parameters, arguments, mock_parameters, mock_arguments, matchers) \
    extern return_type delegate_real_##api_name mock_parameters;
#include <mock_lua_api_table.h>
#undef MOCK_LUA_VOID
#undef MOCK_LUA_RET

#define MOCK_VARIADIC(return_type, original_name, api_name, parameters, arguments, mock_parameters, mock_arguments) \
    extern return_type delegate_real_##api_name mock_parameters;
#include <mock_lua_variadic_api_table.h>
#undef MOCK_VARIADIC

class Mock_lua
{
  public:
#define MOCK_LUA_RET(return_type, original_name, api_name, parameters, arguments, mock_parameters, mock_arguments, matchers)  MOCK_METHOD(return_type, api_name, mock_parameters);
#define MOCK_LUA_VOID(return_type, original_name, api_name, parameters, arguments, mock_parameters, mock_arguments, matchers) MOCK_METHOD(return_type, api_name, mock_parameters);
#include <mock_lua_api_table.h>
#undef MOCK_LUA_VOID
#undef MOCK_LUA_RET

#define MOCK_VARIADIC(return_type, original_name, api_name, parameters, arguments, mock_parameters, mock_arguments) \
    MOCK_METHOD(return_type, api_name, mock_parameters);
#include <mock_lua_variadic_api_table.h>
#undef MOCK_VARIADIC

    Mock_lua();
    virtual ~Mock_lua();
};

extern Mock_lua *_mock_lua;

#endif /* MOCK_LUA_H */
