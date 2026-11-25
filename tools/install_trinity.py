#!/usr/bin/env python3
"""Download, verify and install an official Trinity release into a host's mods folder.

Examples:
    python tools/install_trinity.py --host-dir "X:/fefw-recomp-0.2.13" --version 0.2.0
    python tools/install_trinity.py --host-dir "X:/ObeliskChamber-0.3.2" --version 0.2.0
    python tools/install_trinity.py --mods-dir ./mods --version 0.2.0 --platform linux-x64

The official SHA256SUMS is downloaded first and the selected archive is checked
before anything is extracted. `--bundle` only stages the verified payload for a
host package; it does not install into a running host.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import sys
import tarfile
import tempfile
import urllib.error
import urllib.request
import zipfile
from pathlib import Path, PurePosixPath

REPO = "https://github.com/XiONDiGiTaL/Trinity"
DEFAULT_VERSION = "0.2.0"


def request_bytes(url: str) -> bytes:
    req = urllib.request.Request(url, headers={"User-Agent": "Trinity-installer/1"})
    with urllib.request.urlopen(req, timeout=60) as response:
        return response.read()


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def checksum_for(text: str, name: str) -> str:
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split(None, 1)
        if len(parts) != 2:
            continue
        expected, listed = parts
        if listed.lstrip("*").strip() == name:
            if len(expected) != 64 or any(c not in "0123456789abcdefABCDEF" for c in expected):
                break
            return expected.lower()
    raise SystemExit(f"SHA256SUMS has no valid entry for {name}")


def safe_member(name: str) -> PurePosixPath:
    path = PurePosixPath(name.replace("\\", "/"))
    if path.is_absolute() or not path.parts or path.parts[0] in ("", ".") or ".." in path.parts:
        raise SystemExit(f"unsafe archive path: {name}")
    if any(":" in part for part in path.parts):
        raise SystemExit(f"unsafe archive path: {name}")
    return path


def extract(archive: Path, stage: Path) -> Path:
    if archive.name.endswith(".zip"):
        with zipfile.ZipFile(archive) as z:
            for info in z.infolist():
                path = safe_member(info.filename)
                if info.is_dir():
                    continue
                dest = stage.joinpath(*path.parts)
                dest.parent.mkdir(parents=True, exist_ok=True)
                with z.open(info) as src, dest.open("wb") as out:
                    shutil.copyfileobj(src, out)
    elif archive.name.endswith((".tar.gz", ".tgz")):
        with tarfile.open(archive, "r:gz") as tar:
            for member in tar.getmembers():
                path = safe_member(member.name)
                if member.issym() or member.islnk() or not member.isfile():
                    continue
                dest = stage.joinpath(*path.parts)
                dest.parent.mkdir(parents=True, exist_ok=True)
                src = tar.extractfile(member)
                if src is None:
                    continue
                with src, dest.open("wb") as out:
                    shutil.copyfileobj(src, out)
                if member.name.endswith(".so"):
                    dest.chmod(0o755)
    else:
        raise SystemExit(f"unsupported archive type: {archive.name}")
    payload = stage / "TrinityCore"
    if not payload.is_dir():
        raise SystemExit("the archive has no TrinityCore folder")
    return payload


def installed_version(folder: Path) -> str | None:
    try:
        value = json.loads((folder / "plugin.json").read_text(encoding="utf-8"))
        version = value.get("version")
        return version if isinstance(version, str) else None
    except (OSError, ValueError):
        return None


def version_tuple(value: str) -> tuple[int, int, int]:
    parts = (value.split(".") + ["0", "0", "0"])[:3]
    out = []
    for part in parts:
        try:
            out.append(int(part))
        except ValueError:
            out.append(0)
    return tuple(out)  # type: ignore[return-value]


def install(payload: Path, mods: Path, force: bool) -> Path:
    target = mods / "TrinityCore"
    current = installed_version(target)
    bundled = installed_version(payload)
    if current and bundled and not force and version_tuple(current) >= version_tuple(bundled):
        print(f"kept {target} (installed {current}, release {bundled})")
        return target
    target.mkdir(parents=True, exist_ok=True)
    # Copy over the old payload but never touch .nxp-settings.ini or any other
    # file the player added to the folder.
    for src in payload.rglob("*"):
        if not src.is_file():
            continue
        dest = target / src.relative_to(payload)
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dest)
    if os.name != "nt":
        legacy = target / "exefs" / "trinity.dll"
        if legacy.is_file():
            legacy.unlink()
    print(f"installed Trinity {bundled or 'unknown'} in {target}")
    return target


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--version", default=DEFAULT_VERSION)
    ap.add_argument("--platform", choices=("windows-x64", "linux-x64"), default=("windows-x64" if os.name == "nt" else "linux-x64"))
    ap.add_argument("--host", choices=("emblemweave", "fefw-recomp", "obelisk-chamber"), default="fefw-recomp")
    ap.add_argument("--host-dir", type=Path)
    ap.add_argument("--mods-dir", type=Path)
    ap.add_argument("--bundle", type=Path, help="stage the verified TrinityCore folder here; do not install")
    ap.add_argument("--base-url", help="override the release download root (a URL or local directory)")
    ap.add_argument("--archive", type=Path, help="use a local archive; SHA256SUMS is still required")
    ap.add_argument("--checksums", type=Path, help="local SHA256SUMS; defaults beside --archive")
    ap.add_argument("--force", action="store_true", help="replace an equal or newer install")
    args = ap.parse_args()

    ext = "zip" if args.platform == "windows-x64" else "tar.gz"
    asset_name = f"Trinity-v{args.version}-{args.platform}.{ext}"
    if args.base_url:
        base = args.base_url
        if "://" not in base:
            base = Path(base).resolve().as_uri()
        if not base.endswith("/"):
            base += "/"
    else:
        base = f"{REPO}/releases/download/v{args.version}/"

    with tempfile.TemporaryDirectory(prefix="trinity-install-") as tmp:
        tmp_path = Path(tmp)
        if args.archive:
            archive = args.archive
        else:
            archive = tmp_path / asset_name
            print(f"downloading {base}{asset_name}")
            archive.write_bytes(request_bytes(base + asset_name))
        sums_path = args.checksums or (args.archive.parent / "SHA256SUMS" if args.archive else None)
        if sums_path:
            sums_text = sums_path.read_text(encoding="utf-8")
        else:
            sums_text = request_bytes(base + "SHA256SUMS").decode("utf-8")
        expected = checksum_for(sums_text, asset_name)
        actual = digest(archive)
        if actual != expected:
            raise SystemExit(f"SHA-256 mismatch for {asset_name}: expected {expected}, got {actual}")
        print(f"verified {asset_name} sha256 {actual}")

        stage = tmp_path / "stage"
        stage.mkdir()
        payload = extract(archive, stage)
        if args.bundle:
            args.bundle.mkdir(parents=True, exist_ok=True)
            destination = args.bundle / "TrinityCore"
            if destination.exists():
                shutil.rmtree(destination)
            shutil.copytree(payload, destination)
            print(f"staged verified payload in {destination}")
            return

        if args.mods_dir:
            mods = args.mods_dir
        elif args.host_dir:
            mods = args.host_dir / ("data/play/mods" if args.host == "emblemweave" else "mods")
        else:
            raise SystemExit("pass --host-dir or --mods-dir (or --bundle)")
        mods.mkdir(parents=True, exist_ok=True)
        install(payload, mods, args.force)


if __name__ == "__main__":
    try:
        main()
    except (OSError, urllib.error.URLError) as error:
        print(f"install-trinity: {error}", file=sys.stderr)
        raise SystemExit(1)
