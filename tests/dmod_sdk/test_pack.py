from __future__ import annotations

import importlib.util
import json
import tempfile
import unittest
import zipfile
from pathlib import Path

PACKER_PATH = Path(__file__).resolve().parents[2] / "tools" / "dmod" / "pack.py"
SPEC = importlib.util.spec_from_file_location("dmod_pack", PACKER_PATH)
assert SPEC is not None and SPEC.loader is not None
dmod_pack = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(dmod_pack)


class DModPackerTests(unittest.TestCase):
    def make_lua_mod(self, root: Path) -> None:
        (root / "scripts").mkdir(parents=True)
        (root / "manifest.json").write_text(json.dumps({
            "format_version": 1,
            "id": "test.lua",
            "name": "Lua Test",
            "version": "1.0.0",
            "runtime": "lua",
            "entry_script": "scripts/main.lua",
        }), encoding="utf-8")
        (root / "scripts" / "main.lua").write_text("function on_update(delta) end\n", encoding="utf-8")

    def test_lua_only_package_needs_no_shared_library(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / "mod"
            root.mkdir()
            self.make_lua_mod(root)
            output = Path(temp) / "LuaTest.dmod"
            dmod_pack.build_package(root, output)
            with zipfile.ZipFile(output) as archive:
                self.assertIsNone(archive.testzip())
                self.assertIn("manifest.json", archive.namelist())
                self.assertIn("scripts/main.lua", archive.namelist())
                manifest = json.loads(archive.read("manifest.json"))
                self.assertEqual(manifest["runtime"], "lua")
                self.assertFalse(any(name.endswith(".so") for name in archive.namelist()))

    def test_rejects_traversal_in_entry_script(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / "mod"
            root.mkdir()
            self.make_lua_mod(root)
            manifest_path = root / "manifest.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            manifest["entry_script"] = "../outside.lua"
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            with self.assertRaises(dmod_pack.PackageError):
                dmod_pack.load_manifest(root)

    def test_native_mode_requires_prebuilt_library(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / "mod"
            root.mkdir()
            (root / "manifest.json").write_text(json.dumps({
                "format_version": 1,
                "id": "test.native",
                "name": "Native Test",
                "version": "1.0.0",
                "runtime": "native",
                "native_library": "native/linux-x86_64/libtest.so",
            }), encoding="utf-8")
            with self.assertRaises(dmod_pack.PackageError):
                dmod_pack.load_manifest(root)

    def test_hybrid_includes_lua_and_native_files(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / "mod"
            (root / "scripts").mkdir(parents=True)
            (root / "native" / "android-arm64-v8a").mkdir(parents=True)
            (root / "scripts" / "main.lua").write_text("return true\n", encoding="utf-8")
            (root / "native" / "android-arm64-v8a" / "libtest.so").write_bytes(b"native-test")
            (root / "manifest.json").write_text(json.dumps({
                "format_version": 1,
                "id": "test.hybrid",
                "name": "Hybrid Test",
                "version": "1.0.0",
                "runtime": "hybrid",
                "entry_script": "scripts/main.lua",
                "native_library": "native/android-arm64-v8a/libtest.so",
            }), encoding="utf-8")
            output = Path(temp) / "Hybrid.dmod"
            dmod_pack.build_package(root, output)
            with zipfile.ZipFile(output) as archive:
                self.assertIn("scripts/main.lua", archive.namelist())
                self.assertIn("native/android-arm64-v8a/libtest.so", archive.namelist())


if __name__ == "__main__":
    unittest.main()
