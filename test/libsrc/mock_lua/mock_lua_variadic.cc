#define MOCK_LUA_DISABLE_CALLER_INJECTION
#include <mock_lua.h>
#undef MOCK_LUA_DISABLE_CALLER_INJECTION

#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <type_traits>

namespace
{

using lua_gc_fn = int (*)(lua_State *, int, ...);

lua_gc_fn resolve_lua_gc()
{
    static lua_gc_fn real_fn = reinterpret_cast<lua_gc_fn>(resolveSharedSymbolOrExit(kLibLuaName, "lua_app_lua_gc"));
    return real_fn;
}

template <typename T>
void trace_result(const char *func, const char *caller_file, const int caller_line, const char *caller_function,
                  const T value)
{
    if (getTraceLevel() <= TRACE_NONE)
    {
        return;
    }

    std::printf("  > %s (%s:%d %s)", func, caller_file, caller_line, caller_function);
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

const char *delegate_real_lua_app_lua_pushfstring(const char *caller_file, const int caller_line,
                                                  const char *caller_function, lua_State *state, const char *format,
                                                  va_list args)
{
    return delegate_real_lua_app_lua_pushvfstring(caller_file, caller_line, caller_function, state, format, args);
}

int delegate_real_lua_app_lua_gc(const char *caller_file, const int caller_line, const char *caller_function,
                                 lua_State *state, int operation, va_list args)
{
    (void)caller_file;
    (void)caller_line;
    (void)caller_function;
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

int delegate_real_lua_app_luaL_error(const char *caller_file, const int caller_line, const char *caller_function,
                                     lua_State *state, const char *format, va_list args)
{
    delegate_real_lua_app_luaL_where(caller_file, caller_line, caller_function, state, 1);
    delegate_real_lua_app_lua_pushvfstring(caller_file, caller_line, caller_function, state, format, args);
    delegate_real_lua_app_lua_concat(caller_file, caller_line, caller_function, state, 2);
    return delegate_real_lua_app_lua_error(caller_file, caller_line, caller_function, state);
}

const char *invoke_lua_pushfstring(const char *caller_file, const int caller_line, const char *caller_function,
                                   lua_State *state, const char *format, va_list args)
{
    const char *return_value;
    if (_mock_lua != nullptr)
    {
        return_value =
            _mock_lua->lua_app_lua_pushfstring(caller_file, caller_line, caller_function, state, format, args);
    }
    else
    {
        return_value =
            delegate_real_lua_app_lua_pushfstring(caller_file, caller_line, caller_function, state, format, args);
    }
    trace_result("lua_app_lua_pushfstring", caller_file, caller_line, caller_function, return_value);
    return return_value;
}

int invoke_lua_gc(const char *caller_file, const int caller_line, const char *caller_function, lua_State *state,
                  int operation, va_list args)
{
    int return_value;
    if (_mock_lua != nullptr)
    {
        return_value = _mock_lua->lua_app_lua_gc(caller_file, caller_line, caller_function, state, operation, args);
    }
    else
    {
        return_value = delegate_real_lua_app_lua_gc(caller_file, caller_line, caller_function, state, operation, args);
    }
    trace_result("lua_app_lua_gc", caller_file, caller_line, caller_function, return_value);
    return return_value;
}

int invoke_luaL_error(const char *caller_file, const int caller_line, const char *caller_function, lua_State *state,
                      const char *format, va_list args)
{
    int return_value;
    if (_mock_lua != nullptr)
    {
        return_value = _mock_lua->lua_app_luaL_error(caller_file, caller_line, caller_function, state, format, args);
    }
    else
    {
        return_value = delegate_real_lua_app_luaL_error(caller_file, caller_line, caller_function, state, format, args);
    }
    trace_result("lua_app_luaL_error", caller_file, caller_line, caller_function, return_value);
    return return_value;
}

extern "C" const char *mock_lua_app_lua_pushfstring_variadic(const char *caller_file, const int caller_line,
                                                             const char *caller_function, lua_State *state,
                                                             const char *format, ...)
{
    va_list args;
    va_start(args, format);
    const char *result = invoke_lua_pushfstring(caller_file, caller_line, caller_function, state, format, args);
    va_end(args);
    return result;
}

extern "C" int mock_lua_app_lua_gc_variadic(const char *caller_file, const int caller_line, const char *caller_function,
                                            lua_State *state, int operation, ...)
{
    va_list args;
    va_start(args, operation);
    const int result = invoke_lua_gc(caller_file, caller_line, caller_function, state, operation, args);
    va_end(args);
    return result;
}

extern "C" int mock_lua_app_luaL_error_variadic(const char *caller_file, const int caller_line,
                                                const char *caller_function, lua_State *state, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    const int result = invoke_luaL_error(caller_file, caller_line, caller_function, state, format, args);
    va_end(args);
    return result;
}

#if !defined(_WIN32)
MOCK_WEAK_IMPL(const char *, lua_app_lua_pushfstring, lua_State *state, const char *format, ...)
#else
extern "C" const char *lua_app_lua_pushfstring(lua_State *state, const char *format, ...)
#endif
{
    va_list args;
    va_start(args, format);
    const char *result = invoke_lua_pushfstring("<unknown>", 0, "<unknown>", state, format, args);
    va_end(args);
    return result;
}

#if !defined(_WIN32)
MOCK_WEAK_IMPL(int, lua_app_lua_gc, lua_State *state, int operation, ...)
#else
extern "C" int lua_app_lua_gc(lua_State *state, int operation, ...)
#endif
{
    va_list args;
    va_start(args, operation);
    const int result = invoke_lua_gc("<unknown>", 0, "<unknown>", state, operation, args);
    va_end(args);
    return result;
}

#if !defined(_WIN32)
MOCK_WEAK_IMPL(int, lua_app_luaL_error, lua_State *state, const char *format, ...)
#else
extern "C" int lua_app_luaL_error(lua_State *state, const char *format, ...)
#endif
{
    va_list args;
    va_start(args, format);
    const int result = invoke_luaL_error("<unknown>", 0, "<unknown>", state, format, args);
    va_end(args);
    return result;
}
