#include <testfw.h>

#include <lua.hpp>

#include <set>
#include <string>
#include <type_traits>

#define MOCK_LUA_RET(return_type, name, parameters, arguments, matchers) \
    using expected_##name##_fn = return_type(*) parameters; \
    static_assert(std::is_same<decltype(&name), expected_##name##_fn>::value, #name " のシグネチャが不一致です");
#define MOCK_LUA_VOID(return_type, name, parameters, arguments, matchers) \
    using expected_##name##_fn = return_type(*) parameters; \
    static_assert(std::is_same<decltype(&name), expected_##name##_fn>::value, #name " のシグネチャが不一致です");
#include <mock_lua_api_table.h>
#undef MOCK_LUA_VOID
#undef MOCK_LUA_RET

using expected_lua_pushfstring_fn = const char *(*)(lua_State *, const char *, ...);
using expected_lua_gc_fn = int (*)(lua_State *, int, ...);
using expected_luaL_error_fn = int (*)(lua_State *, const char *, ...);
static_assert(std::is_same<decltype(&lua_pushfstring), expected_lua_pushfstring_fn>::value,
              "lua_pushfstring のシグネチャが不一致です");
static_assert(std::is_same<decltype(&lua_gc), expected_lua_gc_fn>::value, "lua_gc のシグネチャが不一致です");
static_assert(std::is_same<decltype(&luaL_error), expected_luaL_error_fn>::value,
              "luaL_error のシグネチャが不一致です");

static const char *const kExpectedExportNames[] = {
#define MOCK_LUA_RET(return_type, name, parameters, arguments, matchers)  #name,
#define MOCK_LUA_VOID(return_type, name, parameters, arguments, matchers) #name,
#include <mock_lua_api_table.h>
#undef MOCK_LUA_VOID
#undef MOCK_LUA_RET
    "lua_pushfstring",
    "lua_gc",
    "luaL_error",
};

// liblua の公開関数と mock_lua の API 表が一致することの確認
TEST(exportTest, lua_symbols_match_api_table)
{
    // Arrange
    std::set<std::string> expected(
        std::begin(kExpectedExportNames),
        std::end(kExpectedExportNames)); // [状態] - mock_lua の API 表から期待する公開関数名を構築する。
    std::string path = findWorkspaceRoot() + "/app/lua/prod/lib/liblua" +
                       TESTFW_SHARED_LIBRARY_EXTENSION; // [状態] - 検査対象を liblua の動的ライブラリとする。

    // Pre-Assert

    // Act
    std::set<std::string> all_actual =
        testing::getActualExportNames(path); // [手順] - liblua のエクスポート名を取得する。
    std::set<std::string> actual;
    for (const std::string &name : all_actual)
    {
        if (name.rfind("lua_", 0u) == 0u || name.rfind("luaL_", 0u) == 0u || name.rfind("luaopen_", 0u) == 0u)
        {
            actual.insert(name);
        }
    }

    // Assert
    EXPECT_EQ(expected, actual);    // [確認_正常系] - liblua の全公開関数名が mock_lua の API 表と一致すること。
    EXPECT_EQ(156u, actual.size()); // [確認_正常系] - liblua の公開関数数が 156 であること。
}
