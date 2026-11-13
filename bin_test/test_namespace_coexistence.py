#!/usr/bin/env python3
"""Verify app API isolation from an original-name shared library."""
import sys
import unittest
from pathlib import Path

BIN_TEST_PLATFORM = "linux"
APP_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(APP_DIR.parents[1] / "framework" / "testfw" / "bin_internal"))
from shared_namespace_probe import verify_namespace

@unittest.skipUnless(sys.platform.startswith("linux"), "[Linux] ELF namespace probe requires Linux")
class NamespaceCoexistenceTest(unittest.TestCase):
    def test_both_global_load_orders(self):
        verify_namespace(APP_DIR, 'lua_app', 'lua_gettop', '#include <lua.h>\n#include <lauxlib.h>\nint namespace_probe(void) { lua_State *state=lua_app_luaL_newstate(); if(state==0) return 10; lua_app_lua_pushinteger(state,42); int n=lua_app_lua_gettop(state); lua_app_lua_close(state); return n!=1; }\n')

if __name__ == "__main__":
    unittest.main()
