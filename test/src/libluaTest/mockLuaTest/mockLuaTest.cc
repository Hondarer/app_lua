#include <testfw.h>

#include <mock_lua.h>

extern "C"
{
const char *namespace_test_caller_file(void);
int namespace_test_caller_line(void);
int namespace_test_direct_call(lua_State *state);
int namespace_test_pointer_call(lua_State *state);
}
const char *namespace_test_cpp_caller_file();
int namespace_test_cpp_caller_line();
int namespace_test_cpp_direct_call(lua_State *state);

// Mock_lua を生成しない場合に Lua の処理へ委譲されることの確認
TEST(mockLuaTest, delegates_to_real_without_mock)
{
    // Arrange

    // Pre-Assert

    // Act
    lua_State *state = lua_app_luaL_newstate(); // [手順] - Mock_lua を生成しない状態で lua_app_luaL_newstate() を呼び出す。

    // Assert
    ASSERT_NE(nullptr, state); // [確認_正常系] - lua_app_luaL_newstate() の戻り値が nullptr 以外であること。

    // Cleanup
    lua_app_lua_close(state);
}

// 注入済み Mock_lua の未設定呼び出しが Lua の処理へ委譲されることの確認
TEST(mockLuaTest, delegates_to_real_with_default_action)
{
    // Arrange
    NiceMock<Mock_lua> mock_lua; // [状態] - 既定動作の Mock_lua を生成する。

    // Pre-Assert

    // Act
    lua_State *state = lua_app_luaL_newstate(); // [手順] - 既定動作の Mock_lua を介して lua_app_luaL_newstate() を呼び出す。

    // Assert
    ASSERT_NE(nullptr, state); // [確認_正常系] - lua_app_luaL_newstate() の戻り値が nullptr 以外であること。

    // Cleanup
    lua_app_lua_close(state);
}

// EXPECT_CALL により Lua API の戻り値を変更できることの確認
TEST(mockLuaTest, overrides_result)
{
    // Arrange
    NiceMock<Mock_lua> mock_lua; // [状態] - 既定動作の Mock_lua を生成する。
    lua_State *expected =
        reinterpret_cast<lua_State *>(1); // [状態] - モックの戻り値とするダミーの lua_State を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_lua, lua_app_luaL_newstate(_, _, _))
        .WillOnce(Return(expected)); // [Pre-Assert確認_正常系] - lua_app_luaL_newstate() が 1 回呼び出されること。
                                     // [Pre-Assert手順] - lua_app_luaL_newstate() から expected を返す。

    // Act
    lua_State *actual = lua_app_luaL_newstate(); // [手順] - 戻り値を設定した Mock_lua を介して lua_app_luaL_newstate() を呼び出す。

    // Assert
    EXPECT_EQ(expected, actual); // [確認_正常系] - lua_app_luaL_newstate() の戻り値が expected と一致すること。
}

TEST(mockLuaTest, injects_c_callsite_and_marks_function_pointer_as_unknown)
{
    // Arrange
    NiceMock<Mock_lua> mock_lua;

    // Pre-Assert
    EXPECT_CALL(mock_lua, lua_app_lua_gettop(StrEq(namespace_test_caller_file()), namespace_test_caller_line(),
                                            StrEq("namespace_test_direct_call"), nullptr))
        .WillOnce(Return(3)); // [Pre-Assert確認_正常系] - C 呼び出し元の情報を伴って lua_app_lua_gettop を呼び出す。
    EXPECT_CALL(mock_lua, lua_app_lua_gettop(StrEq(namespace_test_cpp_caller_file()), namespace_test_cpp_caller_line(),
                                            StrEq("namespace_test_cpp_direct_call"), nullptr))
        .WillOnce(Return(3)); // [Pre-Assert確認_正常系] - C++ 呼び出し元の情報を伴って lua_app_lua_gettop を呼び出す。
    EXPECT_CALL(mock_lua, lua_app_lua_gettop(StrEq("<unknown>"), 0, StrEq("<unknown>"), nullptr))
        .WillOnce(Return(7)); // [Pre-Assert確認_正常系] - 関数ポインター経由の取得不能値を受け取る。

    // Act
    const int direct_result = namespace_test_direct_call(nullptr);
    const int cpp_direct_result = namespace_test_cpp_direct_call(nullptr);
    const int pointer_result = namespace_test_pointer_call(nullptr);

    // Assert
    EXPECT_EQ(3, direct_result); // [確認_正常系] - C 直接呼び出しの mock 戻り値を受け取る。
    EXPECT_EQ(3, cpp_direct_result); // [確認_正常系] - C++ 直接呼び出しの mock 戻り値を受け取る。
    EXPECT_EQ(7, pointer_result); // [確認_正常系] - 関数ポインター呼び出しの mock 戻り値を受け取る。
}

// 可変長引数を受け取る lua_app_lua_pushfstring の既定動作が Lua の処理へ委譲されることの確認
TEST(mockLuaTest, delegates_variadic_format_to_real)
{
    // Arrange
    NiceMock<Mock_lua> mock_lua;        // [状態] - 既定動作の Mock_lua を生成する。
    lua_State *state = lua_app_luaL_newstate(); // [状態] - 新しい lua_State を用意する。
    ASSERT_NE(nullptr, state);          // [状態確認] - lua_app_luaL_newstate() の戻り値が nullptr 以外であること。

    // Pre-Assert

    // Act
    const char *actual =
        lua_app_lua_pushfstring(state, "%s:%d", "lua", 55); // [手順] - 可変長引数を指定して lua_app_lua_pushfstring() を呼び出す。

    // Assert
    EXPECT_STREQ("lua:55", actual); // [確認_正常系] - lua_app_lua_pushfstring() の戻り値が "lua:55" であること。

    // Cleanup
    lua_app_lua_close(state);
}

// 可変長引数を受け取る lua_app_lua_gc の戻り値を変更できることの確認
TEST(mockLuaTest, overrides_variadic_gc_result)
{
    // Arrange
    NiceMock<Mock_lua> mock_lua;                         // [状態] - 既定動作の Mock_lua を生成する。
    lua_State *state = reinterpret_cast<lua_State *>(1); // [状態] - モック呼び出し用のダミーの lua_State を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_lua, lua_app_lua_gc(_, _, _, state, LUA_GCCOUNT, _))
        .WillOnce(
            Return(123)); // [Pre-Assert確認_正常系] - lua_app_lua_gc() が state、LUA_GCCOUNT を指定して 1 回呼び出されること。
                          // [Pre-Assert手順] - lua_app_lua_gc() から 123 を返す。

    // Act
    int actual = lua_app_lua_gc(state, LUA_GCCOUNT); // [手順] - 戻り値を設定した Mock_lua を介して lua_app_lua_gc() を呼び出す。

    // Assert
    EXPECT_EQ(123, actual); // [確認_正常系] - lua_app_lua_gc() の戻り値が 123 であること。
}

// 実 Lua 状態を作らずに後続 API をスタブして単体隔離できることの確認
TEST(mockLuaTest, isolates_calls_without_real_state)
{
    // Arrange
    NiceMock<Mock_lua> mock_lua;                         // [状態] - 既定動作の Mock_lua を生成する。
    lua_State *state = reinterpret_cast<lua_State *>(1); // [状態] - モック呼び出し用のダミーの lua_State を用意する。
    const char *expected = "value";                      // [状態] - モックの戻り値とする文字列を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_lua, lua_app_luaL_newstate(_, _, _))
        .WillOnce(Return(state)); // [Pre-Assert確認_正常系] - lua_app_luaL_newstate() が 1 回呼び出されること。
                                  // [Pre-Assert手順] - lua_app_luaL_newstate() から state を返す。
    EXPECT_CALL(mock_lua, lua_app_lua_getglobal(_, _, _, state, StrEq("name")))
        .WillOnce(Return(
            LUA_TSTRING)); // [Pre-Assert確認_正常系] - lua_app_lua_getglobal() が state、"name" と等しい文字列を指定して 1 回呼び出されること。
                           // [Pre-Assert手順] - lua_app_lua_getglobal() から LUA_TSTRING を返す。
    EXPECT_CALL(mock_lua, lua_app_lua_tolstring(_, _, _, state, -1, nullptr))
        .WillOnce(Return(
            expected)); // [Pre-Assert確認_正常系] - lua_app_lua_tolstring() が state、-1、nullptr を指定して 1 回呼び出されること。
                        // [Pre-Assert手順] - lua_app_lua_tolstring() から expected を返す。
    EXPECT_CALL(mock_lua, lua_app_lua_close(_, _, _, state))
        .WillOnce(Return()); // [Pre-Assert確認_正常系] - lua_app_lua_close() が state を指定して 1 回呼び出されること。
                             // [Pre-Assert手順] - lua_app_lua_close() から戻る。

    // Act
    lua_State *actual = lua_app_luaL_newstate();         // [手順] - lua_app_luaL_newstate() を呼び出す。
    lua_app_lua_getglobal(actual, "name");               // [手順] - name を指定して lua_app_lua_getglobal() を呼び出す。
    const char *text = lua_app_lua_tostring(actual, -1); // [手順] - スタック トップを指定して lua_app_lua_tostring() を呼び出す。
    lua_app_lua_close(actual);                           // [手順] - actual を指定して lua_app_lua_close() を呼び出す。

    // Assert
    EXPECT_EQ(state, actual);     // [確認_正常系] - lua_app_luaL_newstate() の戻り値が state と一致すること。
    EXPECT_STREQ(expected, text); // [確認_正常系] - lua_app_lua_tostring() の戻り値が expected と一致すること。
}

extern "C" int namespace_test_variadic_line(void);
extern "C" int namespace_test_variadic_call(void);

TEST(mockLuaTest, preserves_variadic_caller_information)
{
    // Arrange
    ::testing::NiceMock<Mock_lua> mock;

    // Pre-Assert
    EXPECT_CALL(mock, lua_app_lua_gc(::testing::StrEq(namespace_test_caller_file()),
        namespace_test_variadic_line(), ::testing::StrEq("namespace_test_variadic_call"), nullptr, LUA_GCCOUNT, ::testing::_))
        .WillOnce(::testing::Return(17)); // [Pre-Assert確認_正常系] - 可変長引数の入口が呼び出し元を維持すること。

    // Act / Assert
    EXPECT_EQ(17, namespace_test_variadic_call()); // [確認_正常系] - 設定した戻り値を返すこと。
}
