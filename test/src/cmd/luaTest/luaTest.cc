#include <testfw.h>

#include <lua.hpp>

class luaTest : public Test
{
};

// dostring が式の評価結果を返すことの確認
TEST_F(luaTest, dostring_returns_evaluated_expression)
{
    // Arrange
    lua_State *L = luaL_newstate(); // [状態] - 新しい lua_State を用意する。
    luaL_openlibs(L);

    // Pre-Assert

    // Act
    int status = luaL_dostring(L, "return 1 + 1"); // [手順] - "return 1 + 1" を指定して luaL_dostring() を呼び出す。

    // Assert
    ASSERT_EQ(LUA_OK, status); // [確認_正常系] - luaL_dostring() の戻り値が LUA_OK であること。
    ASSERT_TRUE(
        lua_isinteger(L, -1)); // [確認_正常系] - スタック トップに対する lua_isinteger() の戻り値が true であること。
    EXPECT_EQ(
        2, lua_tointeger(L, -1)); // [確認_正常系] - スタック トップに対する lua_tointeger() の戻り値が 2 であること。

    // Cleanup
    lua_close(L);
}

// 不正な構文の loadstring がエラーを返すことの確認
TEST_F(luaTest, loadstring_invalid_syntax_returns_error)
{
    // Arrange
    lua_State *L = luaL_newstate(); // [状態] - 新しい lua_State を用意する。
    luaL_openlibs(L);

    // Pre-Assert

    // Act
    int status = luaL_loadstring(
        L, "return 1 +"); // [手順] - 構文が不完全なスクリプト "return 1 +" を指定して luaL_loadstring() を呼び出す。

    // Assert
    EXPECT_EQ(LUA_ERRSYNTAX, status); // [確認_異常系] - luaL_loadstring() の戻り値が LUA_ERRSYNTAX であること。
    EXPECT_NE(
        nullptr,
        lua_tostring(
            L,
            -1)); // [確認_異常系] - スタック トップに対する lua_tostring() の戻り値が nullptr 以外であること。(エラー メッセージが積まれることの確認)

    // Cleanup
    lua_close(L);
}

// グローバル変数の設定と更新が一連で成功することの確認
TEST_F(luaTest, global_variable_roundtrip)
{
    // Arrange
    lua_State *L = luaL_newstate(); // [状態] - 新しい lua_State を用意し、グローバル変数を設定する。
    luaL_openlibs(L);

    // Pre-Assert

    // Act
    lua_pushinteger(L, 42);     // [手順] - 整数値 42 を指定して lua_pushinteger() を呼び出す。
    lua_setglobal(L, "answer"); // [手順] - グローバル変数 "answer" を指定して lua_setglobal() を呼び出す。
    int status = luaL_dostring(
        L,
        "answer = answer + 1"); // [手順] - グローバル変数 answer を 1 加算するスクリプトを指定して luaL_dostring() を呼び出す。
    lua_getglobal(L, "answer"); // [手順] - グローバル変数 "answer" を指定して lua_getglobal() を呼び出す。

    // Assert
    ASSERT_EQ(LUA_OK, status); // [確認_正常系] - luaL_dostring() の戻り値が LUA_OK であること。
    ASSERT_TRUE(
        lua_isinteger(L, -1)); // [確認_正常系] - スタック トップに対する lua_isinteger() の戻り値が true であること。
    EXPECT_EQ(
        43,
        lua_tointeger(
            L,
            -1)); // [確認_正常系] - スタック トップに対する lua_tointeger() の戻り値が 43 であること。(グローバル変数 answer が 42 に 1 を加算した値であることの確認)

    // Cleanup
    lua_close(L);
}
