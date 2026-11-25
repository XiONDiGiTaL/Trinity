# Trinity 0.2.0

This release makes the Windows and Linux plugin binaries first-class, removes
the remaining old panel copy, and adds checked installation paths for all three
hosts.

## Artifacts

- `Trinity-v0.2.0-windows-x64.zip` — `TrinityCore/exefs/Trinity.dll`
- `Trinity-v0.2.0-linux-x64.tar.gz` — `TrinityCore/lib/libTrinity.so` plus the public header
- `SHA256SUMS` and `release.json`

The Windows and Linux files are built from the same NXP source. The Linux file
is an ELF64 x86-64 shared object with `nxp_get_plugin` and `nxmod_init`; it is
not a Windows DLL with a new extension.

## Changes

- Added native Linux builds and a portable `dlopen` host harness.
- Made the canonical outputs `Trinity.dll` and `libTrinity.so`; the manifest,
  tests, adapter and docs use those names.
- Ported host detection to POSIX (`/proc/self/exe`) and kept the Windows module
  path implementation.
- Replaced the old “Three Houses” panel copy with host-integration wording and
  changed the runtime log to “host prefix”.
- Added first-run bundling in EmblemWeave: the launcher embeds 0.2.0, installs
  only `TrinityCore`, refuses to downgrade a newer install and preserves
  `.nxp-settings.ini`.
- Added `tools/package_release.py` and the dependency-free
  `tools/install_trinity.py` (official download, SHA-256 verification and safe
  extraction).
- Added release, compatibility and host-integration documentation.

## Evidence

| Claim | Level | Evidence |
| --- | --- | --- |
| Windows x64 plugin builds and all 5 CTest cases pass | C, R | `Trinity/build/Trinity.dll`; CTest 5/5 |
| Linux x64 `.so` builds, exports both entry points and loads | C, L, R | `build-linux/libTrinity.so`; CTest 5/5; native harness PASS |
| EmblemWeave runtime loads Trinity with two published mods | L, R | `docs/evidence/mod-compat-runtime.log` |
| EmblemWeave bundle installs, preserves settings and guards versions | C, R | `cargo test -p emblemweave trinity_bundle`: 3 passed |
| Official installer verifies SHA-256 before extraction | C, R | local release-artifact install test |

## Compatibility limits

The two published mods, [Quick Save](https://gamebanana.com/mods/742827) and
[Turn Control](https://gamebanana.com/mods/745602), were tested as their original
GameBanana DLLs. They use `NxModApi` directly and do not use the NXP
compatibility adapter. They loaded and initialized alongside Trinity on the
same game build; no save, turn change or gameplay action was exercised.

The Linux result is a native host-harness result. There is no Linux game
launcher or Linux game runtime in this workspace. The panel registered in the
Windows headless runtime, but the in-game panel was not visually captured.

No gameplay changes are included in Trinity Core. See
[INSTALL.md](INSTALL.md) and [COMPATIBILITY.md](COMPATIBILITY.md).
