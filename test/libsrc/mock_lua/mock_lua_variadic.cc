#include <mock_lua.h>

#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <type_traits>

namespace
{

using lua_gc_fn = int (*)(lua_State *, int, ...);

lua_gc_fn resolve_lua_gc()
{
    static lua_gc_fn real_fn = reinterpret_cast<lua_gc_fn>(resolveSharedSymbolOrExit(kLibLuaName, "lua_gc"));
    return real_fn;
}

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

} // namespace

const char *delegate_real_lua_pushfstring(lua_State *state, const char *format, va_list args)
{
    return delegate_real_lua_pushvfstring(state, format, args);
}

int delegate_real_lua_gc(lua_State *state, int operation, va_list args)
{
    lua_gc_fn real_fn = resolve_lua_gc();

    if (operation == LUA_GCSTEP)
    {
        size_t step_size = va_arg(args, size_t);
        return real_fn(state, operation, step_size);
    }
    if (operation == LUA_GCPARAM)
    {
        int parameter = va_arg(args, int);
        int value = va_arg(args, int);
        return real_fn(state, operation, parameter, value);
    }
    return real_fn(state, operation);
}

int delegate_real_luaL_error(lua_State *state, const char *format, va_list args)
{
    delegate_real_luaL_where(state, 1);
    delegate_real_lua_pushvfstring(state, format, args);
    delegate_real_lua_concat(state, 2);
    return delegate_real_lua_error(state);
}

#if !defined(_WIN32)
MOCK_WEAK_IMPL(const char *, lua_pushfstring, lua_State *state, const char *format, ...)
#else
extern "C" const char *lua_pushfstring(lua_State *state, const char *format, ...)
#endif
{
    va_list args;
    const char *return_value;

    va_start(args, format);
    if (_mock_lua != nullptr)
    {
        return_value = _mock_lua->lua_pushfstring(state, format, args);
    }
    else
    {
        return_value = delegate_real_lua_pushfstring(state, format, args);
    }
    va_end(args);
    trace_result(__func__, return_value);
    return return_value;
}

#if !defined(_WIN32)
MOCK_WEAK_IMPL(int, lua_gc, lua_State *state, int operation, ...)
#else
extern "C" int lua_gc(lua_State *state, int operation, ...)
#endif
{
    va_list args;
    int return_value;

    va_start(args, operation);
    if (_mock_lua != nullptr)
    {
        return_value = _mock_lua->lua_gc(state, operation, args);
    }
    else
    {
        return_value = delegate_real_lua_gc(state, operation, args);
    }
    va_end(args);
    trace_result(__func__, return_value);
    return return_value;
}

#if !defined(_WIN32)
MOCK_WEAK_IMPL(int, luaL_error, lua_State *state, const char *format, ...)
#else
extern "C" int luaL_error(lua_State *state, const char *format, ...)
#endif
{
    va_list args;
    int return_value;

    va_start(args, format);
    if (_mock_lua != nullptr)
    {
        return_value = _mock_lua->luaL_error(state, format, args);
    }
    else
    {
        return_value = delegate_real_luaL_error(state, format, args);
    }
    va_end(args);
    trace_result(__func__, return_value);
    return return_value;
}
