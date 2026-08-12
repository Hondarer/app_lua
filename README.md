# lua

このリポジトリ (ディレクトリ) は [c-modernization-kit](https://github.com/Hondarer/c-modernization-kit) の `app/lua` として管理される、個別アプリです。  
c-modernization-kit のワークスペース内 (`framework/makefw` 等と組み合わせた make ビルド環境) で利用することを前提としたアプリであり、本ディレクトリ単体ではビルドできません。

## 概要

[Lua](https://www.lua.org/) のコア VM・標準ライブラリ (`src/` 配下の `.c`/`.h`。  
`lua.c`/`luac.c` を除く) および公式インタープリター (`lua.c`) を、c-modernization-kit の makefw 規約に沿って取り込んだラッパー ライブラリです。Lua 自体のソースは改変せず、リリース アーカイブをそのまま展開して利用します。バイトコード コンパイラ (`luac.c`) は  
取り込みません。

ビルド成果物は動的ライブラリと、`lua.c` から生成する `lua` コマンド (公式インタープリター) です。

- Linux: `liblua.so`
- Windows: `liblua.dll` およびリンク用の import library `liblua.lib`

静的ライブラリ (`liblua.a` および静的リンク用の `liblua.lib`) は生成しません。

`lua` コマンドは行編集オプション (`LUA_USE_READLINE`) を定義しない最小構成でビルドしており、`readline`/`editline` 等の外部ライブラリには依存しません。Linux ビルドでは、Lua 公式ビルドの `linux` ターゲットに合わせて `LUA_USE_LINUX` を定義しています。

C++ から利用する場合は、`lua.h` に `extern "C"` 宣言が含まれていないため、個別ヘッダーではなく `<lua.hpp>` を include してください (`test/src/cmd/luaTest/` を参照)。

## パッケージの配置手順 (初回セットアップ)

本リポジトリは Lua のソース アーカイブ (tar.gz) を Git 管理下に手動配置する運用です。  
`app/lua/packages/` に、以下の手順で取得した tar.gz を 1 つだけ配置してください。

```sh
curl -L -o app/lua/packages/lua-5.5.0.tar.gz \
  https://www.lua.org/ftp/lua-5.5.0.tar.gz
```

最新版は [https://www.lua.org/download.html](https://www.lua.org/download.html) でも確認できます。

配置後、`make` (または `make test`) を実行すると、`app/lua/bin/extract_package.py` が自動的に `prod/include/`、`prod/libsrc/lua/`、`prod/src/cmd/lua/` へ展開します。  
展開先はいずれも生成物であり `.gitignore` 対象です。

`packages/` にアーカイブが存在しない状態で `make` を実行すると、ビルドはエラーで停止し、配置方法の案内が表示されます。

`packages/` に複数のアーカイブが存在する場合はエラーにはせず、ファイル名のバージョン番号が最も新しいものを自動的に採用します (バージョン番号が読み取れないファイルが混在する場合は、更新日時が最も新しいものを採用します)。  
この場合、採用されなかったアーカイブを削除するよう警告が表示されるので、単一ファイル運用に戻してください。

## バージョン更新手順

1. 新しいバージョンの tar.gz を取得し、`app/lua/packages/` に追加する。
2. 古いバージョンの tar.gz を削除する (`packages/` には常に 1 個のみを置く運用)。
3. `make` を実行すると、新しい tar.gz のタイムスタンプが展開済み生成物より新しいと判定され、自動的に再展開されます。

## ライセンス

Lua 本体は MIT License です。単体の `LICENSE` ファイルはアーカイブ内に同梱されておらず、`doc/readme.html` にライセンス条文が記載されています。詳細は [https://www.lua.org/license.html](https://www.lua.org/license.html) を参照してください。

`app/lua` 直下の `LICENSE` (MIT License) は、本ディレクトリのラッパー コード (`bin/extract_package.py`、`makefile`/`makepart.mk` 等の手書きファイル) に対する著作権表示であり、Lua 本体 (`src/` 由来のファイル) には適用されません。

## サンプルとテスト

- `prod/src/cmd/lua/` : `lua.c` (生成物) から構築する公式インタープリター
- `test/src/cmd/luaTest/` : 動作テスト
- `test/src/libluaTest/mockLuaTest/` : Lua API モックの動作テスト
- `test/src/libluaTest/exportTest/` : 公開関数のモック対象漏れを検出するテスト

## Lua API モック

Lua を利用するアプリの単体テストでは、`<mock_lua.h>` と `libmock_lua` を使用できます。  
テスト対象の `makepart.mk` では、`lua` の代わりに `mock_lua` をリンクします。  
`lua` と `mock_lua` を同時にリンクしないでください。Linux では実ライブラリの強シンボルが弱定義のモックを上書きし、`EXPECT_CALL` が効かなくなります。

```makefile
ifdef PLATFORM_WINDOWS
    DEFINES += LUA_CORE
endif

LIBS += mock_lua
```

Windows では、`LUA_CORE` により Lua の DLL import 宣言を無効にし、`mock_lua` が提供する実シンボルを参照します。  
`Mock_lua` を生成しない場合と、生成後に `EXPECT_CALL` や `ON_CALL` を設定しない場合は、動的ライブラリの実関数を呼び出します。  
実関数への委譲では、Linux の `LD_LIBRARY_PATH` または Windows の `PATH` から `liblua` を読み込みます。

振る舞いを変更するテストでは、`Mock_lua` を生成して `EXPECT_CALL` または `ON_CALL` を指定します。

```cpp
NiceMock<Mock_lua> mock_lua;
EXPECT_CALL(mock_lua, luaL_newstate()).WillOnce(Return(nullptr));
```

実の `lua_State` を作らずに単体隔離する場合は、SUT が呼び出す関数をすべてスタブしてください。  
スタブしていない呼び出しは実関数へ委譲されるため、偽ポインターを渡すと実関数が失敗します。

`lua_pushfstring`、`lua_gc`、`luaL_error` は、可変長引数を `va_list` としてモックへ渡します。  
引数の内容を照合しない場合は、該当する `va_list` 引数に `_` を指定してください。

Lua ヘッダーでマクロとして提供される API は、展開後に呼び出される関数をモックしてください。

| マクロ | 展開先 |
|---|---|
| `lua_pop` | `lua_settop` |
| `lua_tostring` | `lua_tolstring` |
| `lua_tonumber` | `lua_tonumberx` |
| `lua_pcall` | `lua_pcallk` |
| `lua_call` | `lua_callk` |
| `lua_newtable` | `lua_createtable` |
| `lua_pushcfunction` | `lua_pushcclosure` |
| `luaL_openlibs` | `luaL_openselectedlibs` |
| `luaL_dostring` | `luaL_loadstring` と `lua_pcallk` |

`lua_getextraspace` と `luaL_addchar` などの Buffer マクロは、`lua_State` や `luaL_Buffer` の領域を直接操作します。  
偽ポインターを返す場合は、これらのマクロを使わないか、実オブジェクトを用意してください。
