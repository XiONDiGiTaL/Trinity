#!/usr/bin/env python3
"""Validate a Trinity plugin manifest without third-party dependencies.

The manifest is launcher metadata, not a security boundary. The runtime still
validates the loaded module and its NXP descriptor.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

SEMVER = re.compile(r"^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?$")
PLUGIN_ID = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{2,127}$")
CAPABILITY = re.compile(r"^[a-z0-9][a-z0-9-]*\.[a-z0-9][a-z0-9.-]*$")
KNOWN_HOSTS = {"emblemweave", "fefw-recomp", "obelisk-chamber"}
PLATFORMS = {"windows", "linux", "macos"}


def safe_relative(path: str) -> bool:
    if not isinstance(path, str) or not path:
        return False
    if path.startswith(("/", "\\")) or re.match(r"^[A-Za-z]:", path):
        return False
    parts = re.split(r"[\\/]", path)
    return all(part not in ("", ".", "..") for part in parts)


def validate(data: object, manifest_dir: Path | None = None) -> list[str]:
    errors: list[str] = []
    if not isinstance(data, dict):
        return ["manifest root must be an object"]

    if data.get("schema") != 1:
        errors.append("schema must be 1")
    plugin_id = data.get("id")
    if not isinstance(plugin_id, str) or not PLUGIN_ID.match(plugin_id):
        errors.append("id must be a stable identifier of 3 to 128 characters")
    for key in ("name", "description"):
        if not isinstance(data.get(key), str) or not data.get(key, "").strip():
            errors.append(f"{key} must be a non-empty string")
    version = data.get("version")
    if not isinstance(version, str) or not SEMVER.match(version):
        errors.append("version must be semantic versioning")
    if not isinstance(data.get("authors"), list) or not data.get("authors"):
        errors.append("authors must be a non-empty array")
    if not isinstance(data.get("license"), str) or not data.get("license"):
        errors.append("license must be a non-empty SPDX expression")

    abi = data.get("abi")
    if not isinstance(abi, dict):
        errors.append("abi must be an object with min and max")
    else:
        lo, hi = abi.get("min"), abi.get("max")
        if not isinstance(lo, int) or not isinstance(hi, int) or lo < 1 or hi < lo:
            errors.append("abi.min and abi.max must be positive integers with min <= max")

    hosts = data.get("hosts")
    if not isinstance(hosts, list) or not hosts:
        errors.append("hosts must be a non-empty array")
    else:
        for host in hosts:
            if host not in KNOWN_HOSTS:
                errors.append(f"unknown host id: {host}")

    entry = data.get("entry")
    if not isinstance(entry, dict) or not entry:
        errors.append("entry must be a non-empty object")
    else:
        for platform, path in entry.items():
            if platform not in PLATFORMS:
                errors.append(f"unknown entry platform: {platform}")
            if not safe_relative(path):
                errors.append(f"entry.{platform} must be a safe relative path")
            elif manifest_dir and not (manifest_dir / path).is_file():
                errors.append(f"entry.{platform} does not exist: {path}")
        if "windows" not in entry and "linux" not in entry:
            errors.append("entry must provide windows or linux")

    capabilities = data.get("capabilities")
    if not isinstance(capabilities, list):
        errors.append("capabilities must be an array")
    else:
        for capability in capabilities:
            if not isinstance(capability, str) or not CAPABILITY.match(capability):
                errors.append(f"invalid capability name: {capability}")

    dependencies = data.get("dependencies", [])
    if not isinstance(dependencies, list):
        errors.append("dependencies must be an array")
    else:
        for dependency in dependencies:
            if not isinstance(dependency, str) or not dependency:
                errors.append("every dependency must be a non-empty id")

    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--allow-missing-entry", action="store_true",
                        help="validate metadata without requiring the binary to exist")
    args = parser.parse_args(argv)
    try:
        data = json.loads(args.manifest.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 2
    errors = validate(data, None if args.allow_missing_entry else args.manifest.parent)
    if errors:
        for error in errors:
            print(f"FAIL: {error}", file=sys.stderr)
        return 1
    print(f"PASS: {args.manifest} is a valid Trinity plugin manifest")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
