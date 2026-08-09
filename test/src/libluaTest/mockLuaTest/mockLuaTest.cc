#include <testfw.h>

#include <mock_lua.h>

// Mock_lua を生成しない場合に Lua の実関数へ委譲されることの確認
TEST(mockLuaTest, delegates_to_real_without_mock)
{
    // Arrange

    // Pre-Assert

    // Act
    lua_State *state = luaL_newstate(); // [手順] - Mock_lua を生成せず luaL_newstate を呼び出す。

    // Assert
    ASSERT_NE(nullptr, state); // [確認_正常系] - luaL_newstate の戻り値が NULL でないこと。

    // Cleanup
    lua_close(state);
}

// 注入済み Mock_lua の未設定呼び出しが Lua の実関数へ委譲されることの確認
TEST(mockLuaTest, delegates_to_real_with_default_action)
{
    // Arrange
    NiceMock<Mock_lua> mock_lua;

    // Pre-Assert

    // Act
    lua_State *state = luaL_newstate(); // [手順] - 既定動作の Mock_lua を介して luaL_newstate を呼び出す。

    // Assert
    ASSERT_NE(nullptr, state); // [確認_正常系] - luaL_newstate の戻り値が NULL でないこと。

    // Cleanup
    lua_close(state);
}

// EXPECT_CALL により Lua API の戻り値を変更できることの確認
TEST(mockLuaTest, overrides_result)
{
    // Arrange
    NiceMock<Mock_lua> mock_lua;
    lua_State *expected = reinterpret_cast<lua_State *>(1);

    // Pre-Assert
    EXPECT_CALL(mock_lua, luaL_newstate())
        .WillOnce(Return(expected)); // [Pre-Assert確認_正常系] - luaL_newstate が 1 回呼び出されること。
                                     // [Pre-Assert手順] - luaL_newstate から expected を返却する。

    // Act
    lua_State *actual = luaL_newstate(); // [手順] - 戻り値を設定した Mock_lua を介して luaL_newstate を呼び出す。

    // Assert
    EXPECT_EQ(expected, actual); // [確認_正常系] - luaL_newstate の戻り値が expected であること。
}

// 可変長引数を受け取る lua_pushfstring の既定動作が実関数へ委譲されることの確認
TEST(mockLuaTest, delegates_variadic_format_to_real)
{
    // Arrange
    NiceMock<Mock_lua> mock_lua;
    lua_State *state = luaL_newstate();

    // Pre-Assert
    ASSERT_NE(nullptr, state); // [Pre-Assert確認_正常系] - luaL_newstate の戻り値が NULL でないこと。

    // Act
    const char *actual =
        lua_pushfstring(state, "%s:%d", "lua", 55); // [手順] - 可変長引数を指定して lua_pushfstring を呼び出す。

    // Assert
    EXPECT_STREQ("lua:55", actual); // [確認_正常系] - lua_pushfstring の戻り値が "lua:55" であること。

    // Cleanup
    lua_close(state);
}

// 可変長引数を受け取る lua_gc の戻り値を変更できることの確認
TEST(mockLuaTest, overrides_variadic_gc_result)
{
    // Arrange
    NiceMock<Mock_lua> mock_lua;
    lua_State *state = reinterpret_cast<lua_State *>(1);

    // Pre-Assert
    EXPECT_CALL(mock_lua, lua_gc(state, LUA_GCCOUNT, _))
        .WillOnce(Return(123)); // [Pre-Assert確認_正常系] - lua_gc が LUA_GCCOUNT を指定して 1 回呼び出されること。
                                // [Pre-Assert手順] - lua_gc から 123 を返却する。

    // Act
    int actual = lua_gc(state, LUA_GCCOUNT); // [手順] - 戻り値を設定した Mock_lua を介して lua_gc を呼び出す。

    // Assert
    EXPECT_EQ(123, actual); // [確認_正常系] - lua_gc の戻り値が 123 であること。
}
