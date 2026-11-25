#!/usr/bin/env python3
"""Build the two public Trinity plugin packages and their checksums.

    python tools/package_release.py --version 0.2.0 \
        --windows build/Trinity.dll --linux build-linux/libTrinity.so --out dist

The Windows archive is a ready-to-copy `TrinityCore` mod folder. The Linux
archive carries the native library, plugin manifest and public header. Both
archives include the install notes and compatibility limits; the sidecar
SHA256SUMS is what the installer and host release checks verify.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import tarfile
import tempfile
import zipfile
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def add_zip_tree(zipf: zipfile.ZipFile, src: Path, arc_root: str = "") -> None:
    for item in sorted(src.rglob("*")):
        if item.is_file():
            rel = item.relative_to(src).as_posix()
            zipf.write(item, f"{arc_root}{rel}" if arc_root else rel)


def add_tar_tree(tar: tarfile.TarFile, src: Path, arc_root: str = "") -> None:
    for item in sorted(src.rglob("*")):
        if item.is_file():
            rel = item.relative_to(src).as_posix()
            info = tar.gettarinfo(str(item), f"{arc_root}{rel}" if arc_root else rel)
            info.uid = info.gid = 0
            info.uname = info.gname = "root"
            info.mtime = int(os.environ.get("SOURCE_DATE_EPOCH", info.mtime))
            if item.suffix == ".so":
                info.mode = 0o755
            with item.open("rb") as f:
                tar.addfile(info, f)


def stage_common(version: str, plugin_dir: Path) -> None:
    plugin_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(ROOT / "plugins" / "trinity_core" / "plugin.json", plugin_dir / "plugin.json")
    (plugin_dir / "emblemweave.json").write_text(
        json.dumps(
            {
                "title": "Trinity Core",
                "author": "XiONDiGiTaL",
                "version": version,
                "description": "Shared NXP services for EmblemWeave, fefw-recomp and ObeliskChamber: host discovery, settings, a native panel and a frame hook.",
                "source": "https://github.com/XiONDiGiTaL/Trinity",
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )
    for name in ("README.md", "LICENSE"):
        src = ROOT / name
        if src.is_file():
            shutil.copy2(src, plugin_dir / name)
    docs = plugin_dir / "docs"
    docs.mkdir(exist_ok=True)
    for name in ("INSTALL.md", "COMPATIBILITY.md", "NXP.md"):
        src = ROOT / "docs" / name
        if src.is_file():
            shutil.copy2(src, docs / name)
    evidence = docs / "evidence"
    evidence.mkdir(exist_ok=True)
    for name in ("fefw-runtime.log", "mod-compat-runtime.log", "linux-host.txt"):
        src = ROOT / "docs" / "evidence" / name
        if src.is_file():
            shutil.copy2(src, evidence / name)


def build_windows(version: str, dll: Path, out: Path) -> Path:
    if dll.read_bytes()[:2] != b"MZ":
        raise SystemExit(f"{dll} is not a PE image")
    archive = out / f"Trinity-v{version}-windows-x64.zip"
    with tempfile.TemporaryDirectory(prefix="trinity-win-") as tmp:
        root = Path(tmp) / "TrinityCore"
        stage_common(version, root)
        (root / "exefs").mkdir(exist_ok=True)
        shutil.copy2(dll, root / "exefs" / "Trinity.dll")
        with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as z:
            add_zip_tree(z, root, "TrinityCore/")
    return archive


def build_linux(version: str, so: Path, out: Path) -> Path:
    if so.read_bytes()[:4] != b"\x7fELF":
        raise SystemExit(f"{so} is not an ELF shared object")
    archive = out / f"Trinity-v{version}-linux-x64.tar.gz"
    with tempfile.TemporaryDirectory(prefix="trinity-linux-") as tmp:
        root = Path(tmp) / "TrinityCore"
        stage_common(version, root)
        (root / "lib").mkdir(exist_ok=True)
        shutil.copy2(so, root / "lib" / "libTrinity.so")
        (root / "include" / "nxp").mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / "include" / "nxp" / "nxp.h", root / "include" / "nxp" / "nxp.h")
        with tarfile.open(archive, "w:gz") as tar:
            add_tar_tree(tar, root, "TrinityCore/")
    return archive


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--version", required=True)
    ap.add_argument("--windows", required=True, type=Path)
    ap.add_argument("--linux", required=True, type=Path)
    ap.add_argument("--out", default="dist", type=Path)
    args = ap.parse_args()

    manifest = json.loads((ROOT / "plugins" / "trinity_core" / "plugin.json").read_text(encoding="utf-8"))
    if manifest.get("version") != args.version:
        raise SystemExit(f"plugin.json says {manifest.get('version')}, package says {args.version}")
    if manifest.get("entry", {}).get("windows") != "Trinity.dll":
        raise SystemExit("plugin.json does not name Trinity.dll")
    if manifest.get("entry", {}).get("linux") != "libTrinity.so":
        raise SystemExit("plugin.json does not name libTrinity.so")
    if not args.windows.is_file() or not args.linux.is_file():
        raise SystemExit("both the Windows DLL and Linux shared object are required")

    args.out.mkdir(parents=True, exist_ok=True)
    win = build_windows(args.version, args.windows, args.out)
    linux = build_linux(args.version, args.linux, args.out)
    sums = args.out / "SHA256SUMS"
    sums.write_text("\n".join(f"{sha256(p)}  {p.name}" for p in (win, linux)) + "\n", encoding="utf-8")
    metadata = {
        "version": args.version,
        "tag": f"v{args.version}",
        "repository": "https://github.com/XiONDiGiTaL/Trinity",
        "built_utc": datetime.now(timezone.utc).replace(microsecond=0).isoformat(),
        "architectures": ["windows-x64", "linux-x64"],
        "abi": manifest.get("abi"),
        "artifacts": [
            {
                "name": p.name,
                "bytes": p.stat().st_size,
                "sha256": sha256(p),
            }
            for p in (win, linux)
        ],
    }
    (args.out / "release.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    print(sums.read_text(encoding="utf-8"), end="")
    print(args.out / "release.json")


if __name__ == "__main__":
    main()
