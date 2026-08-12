ifdef PLATFORM_LINUX
    # Mock_lua::Mock_lua() コンストラクターが多数の ON_CALL を持つため、GCC の
    # -fvar-tracking-assignments が内部サイズ制限を超え、以下のメッセージが出力されることがある。
    #
    # mock_lua.cc:75:1: 備考: 変数追跡サイズ制限が -fvar-tracking-assignments を超過しています。
    # -fvar-tracking-assignments 無しで再度試みています
    #
    # var-tracking-assignments-dom-ops-limit に対応している GCC では、上限値を引き上げて備考を抑制する。
    # gcc 8 系のように未対応の環境では、-fno-var-tracking-assignments に切り替えて回避する。
    GCC_VAR_TRACKING_ASSIGNMENTS_DOM_OPS_LIMIT_SUPPORTED := $(shell $(CXX) -Q --help=params 2>/dev/null | grep -q '^[[:space:]]*var-tracking-assignments-dom-ops-limit[[:space:]]' && echo 1)
    ifneq ($(GCC_VAR_TRACKING_ASSIGNMENTS_DOM_OPS_LIMIT_SUPPORTED),)
        CXXFLAGS += --param=var-tracking-assignments-dom-ops-limit=400000
    else
        CXXFLAGS += -fno-var-tracking-assignments
    endif
endif

ifdef PLATFORM_WINDOWS
    # Lua の DLL import ではなくモックの実シンボルを定義する。
    DEFINES += LUA_CORE

    # 多数の MOCK_METHOD / ON_CALL を含むオブジェクトを COFF の通常上限内へ収める必要がないようにする。
    CXXFLAGS += /bigobj
endif
