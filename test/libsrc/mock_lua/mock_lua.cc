#include <mock_lua.h>

#include <cstdio>
#include <type_traits>

Mock_lua *_mock_lua = nullptr;

namespace
{

template <typename T> void trace_result(const char *func, const T value)
{
    if (getTraceLevel() <= TRACE_NONE)
    {
        return;
    }

    std::printf("  > %s", func);
    if (getTraceLevel() >= TRACE_DETAIL)
    {
        if constexpr (std::is_pointer_v<T>)
        {
            std::printf(" -> 0x%p\n", (const void *)value);
        }
        else if constexpr (std::is_floating_point_v<T>)
        {
            std::printf(" -> %f\n", (double)value);
        }
        else
        {
            std::printf(" -> %lld\n", (long long)value);
        }
    }
    else
    {
        std::printf("\n");
    }
}

void trace_void(const char *func)
{
    if (getTraceLevel() > TRACE_NONE)
    {
        std::printf("  > %s\n", func);
    }
}

} // namespace

#if !defined(_WIN32)
    #define MOCK_LUA_IMPL(return_type, name, ...) MOCK_WEAK_IMPL(return_type, name, __VA_ARGS__)
#else
    #define MOCK_LUA_IMPL(return_type, name, ...) extern "C" return_type name(__VA_ARGS__)
#endif

#define MOCK_LUA_EXPAND(...) __VA_ARGS__

#define MOCK_LUA_RET(return_type, name, parameters, arguments, matchers) \
    return_type delegate_real_##name parameters \
    { \
        static auto real_fn = reinterpret_cast<decltype(&name)>(resolveSharedSymbolOrExit(kLibLuaName, #name)); \
        return real_fn arguments; \
    } \
    MOCK_LUA_IMPL(return_type, name, MOCK_LUA_EXPAND parameters) \
    { \
        return_type return_value; \
        if (_mock_lua != nullptr) \
        { \
            return_value = _mock_lua->name arguments; \
        } \
        else \
        { \
            return_value = delegate_real_##name arguments; \
        } \
        trace_result(__func__, return_value); \
        return return_value; \
    }

#define MOCK_LUA_VOID(return_type, name, parameters, arguments, matchers) \
    return_type delegate_real_##name parameters \
    { \
        static auto real_fn = reinterpret_cast<decltype(&name)>(resolveSharedSymbolOrExit(kLibLuaName, #name)); \
        real_fn arguments; \
    } \
    MOCK_LUA_IMPL(return_type, name, MOCK_LUA_EXPAND parameters) \
    { \
        if (_mock_lua != nullptr) \
        { \
            _mock_lua->name arguments; \
        } \
        else \
        { \
            delegate_real_##name arguments; \
        } \
        trace_void(__func__); \
    }

#include <mock_lua_api_table.h>

#undef MOCK_LUA_VOID
#undef MOCK_LUA_RET
#undef MOCK_LUA_EXPAND
#undef MOCK_LUA_IMPL

Mock_lua::Mock_lua()
{
#define MOCK_LUA_RET(return_type, name, parameters, arguments, matchers) \
    ON_CALL(*this, name matchers).WillByDefault(Invoke(delegate_real_##name));
#define MOCK_LUA_VOID(return_type, name, parameters, arguments, matchers) \
    ON_CALL(*this, name matchers).WillByDefault(Invoke(delegate_real_##name));
#include <mock_lua_api_table.h>
#undef MOCK_LUA_VOID
#undef MOCK_LUA_RET

    ON_CALL(*this, lua_pushfstring(_, _, _)).WillByDefault(Invoke(delegate_real_lua_pushfstring));
    ON_CALL(*this, lua_gc(_, _, _)).WillByDefault(Invoke(delegate_real_lua_gc));
    ON_CALL(*this, luaL_error(_, _, _)).WillByDefault(Invoke(delegate_real_luaL_error));
    _mock_lua = this;
}

Mock_lua::~Mock_lua()
{
    _mock_lua = nullptr;
}
