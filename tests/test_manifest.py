import json
import tempfile
import unittest
from pathlib import Path

from tools.validate_plugin import validate


def base_manifest():
    return {
        "schema": 1,
        "id": "org.xion.trinity.core",
        "name": "Trinity Core",
        "version": "0.2.0",
        "description": "Reference plugin",
        "authors": ["XiONDiGiTaL"],
        "license": "GPL-3.0-or-later",
        "abi": {"min": 1, "max": 1},
        "hosts": ["emblemweave", "fefw-recomp", "obelisk-chamber"],
        "entry": {"windows": "Trinity.dll"},
        "capabilities": ["nxp.ui", "nxp.settings"],
        "dependencies": [],
    }


class ManifestTests(unittest.TestCase):
    def test_repository_manifest_is_valid_without_binary(self):
        path = Path("plugins/trinity_core/plugin.json")
        self.assertEqual(validate(json.loads(path.read_text(encoding="utf-8"))), [])

    def test_base_manifest_is_valid(self):
        self.assertEqual(validate(base_manifest()), [])

    def test_rejects_unknown_host(self):
        manifest = base_manifest()
        manifest["hosts"] = ["unknown-host"]
        self.assertTrue(any("unknown host" in error for error in validate(manifest)))

    def test_rejects_path_traversal(self):
        manifest = base_manifest()
        manifest["entry"]["windows"] = "../../escape.dll"
        self.assertTrue(any("safe relative path" in error for error in validate(manifest)))

    def test_rejects_bad_abi_range(self):
        manifest = base_manifest()
        manifest["abi"] = {"min": 2, "max": 1}
        self.assertTrue(validate(manifest))

    def test_checks_entry_file_when_requested(self):
        manifest = base_manifest()
        with tempfile.TemporaryDirectory() as directory:
            errors = validate(manifest, Path(directory))
            self.assertTrue(any("does not exist" in error for error in errors))
            (Path(directory) / "Trinity.dll").write_bytes(b"not a real dll")
            self.assertEqual(validate(manifest, Path(directory)), [])

    def test_rejects_unprefixed_capability(self):
        manifest = base_manifest()
        manifest["capabilities"] = ["ui"]
        self.assertTrue(validate(manifest))


if __name__ == "__main__":
    unittest.main()
