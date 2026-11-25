# Install Trinity 0.2.0

End users do not need Visual Studio, CMake, a compiler, an SDK or any other
development tool. Use the published archives or the installer below.

Launcher maintainers integrating Trinity into a host build should read
[HOST-INTEGRATION.md](HOST-INTEGRATION.md) for loading, version checks, updates
and error handling.

## Official release assets

| Platform | Archive | Payload |
| --- | --- | --- |
| Windows x64 | `Trinity-v0.2.0-windows-x64.zip` | `TrinityCore/exefs/Trinity.dll` |
| Linux x64 | `Trinity-v0.2.0-linux-x64.tar.gz` | `TrinityCore/lib/libTrinity.so` and `include/nxp/nxp.h` |

`SHA256SUMS` sits beside the archives. The install helper downloads that file
first and refuses an archive whose SHA-256 does not match:

```bash
curl -fLO https://github.com/XiONDiGiTaL/Trinity/releases/download/v0.2.0/SHA256SUMS
sha256sum -c SHA256SUMS
```

## Drop-in install: launcher root or plugins/

Launchers that include the discovery integration accept a payload beside their
own executable. Place the DLL in either place:

```text
<launcher>\Trinity.dll
<launcher>\plugins\Trinity.dll
```

A package folder also works:

```text
<launcher>\TrinityCore\exefs\Trinity.dll
<launcher>\TrinityCore\plugin.json
<launcher>\plugins\TrinityCore\exefs\Trinity.dll
<launcher>\plugins\TrinityCore\plugin.json
```

The launcher stages the payload into the game runtime's existing mods folder
and never loads the DLL itself. The runtime loads it on Play through its normal
code-mod loader.

| Host | Drop-in locations | Staged to |
| --- | --- | --- |
| EmblemWeave | `E:\EmblemWeave\Trinity.dll`, `E:\EmblemWeave\plugins\Trinity.dll` | `data\play\mods\TrinityCore\exefs\Trinity.dll` |
| fefw-recomp | `<install>\Trinity.dll`, `<install>\plugins\Trinity.dll` | `<install>\mods\TrinityCore\exefs\Trinity.dll` |
| ObeliskChamber | `<install>\Trinity.dll`, `<install>\plugins\Trinity.dll` | `<install>\mods\TrinityCore\exefs\Trinity.dll` |

A bare DLL is enough; `plugin.json` is copied when supplied. An installed newer
version is not downgraded by an older package, and `.nxp-settings.ini` is
preserved. Launcher maintainers can use the prepared module and patches in
[integrations/launcher-plugins](../integrations/launcher-plugins/README.md).

This path was verified on all three hosts on 25 November 2026: root and
`plugins/` placements (including a nested `TrinityCore/exefs` package) restored
the 0.2.0 payload, and each game runtime logged `Trinity Core 0.2.0 on
<host>`. The same EmblemWeave build at `X:\EmblemWeave` staged a root DLL as
well; that copy has no game INI, so its runtime was not started.

## fefw-recomp and ObeliskChamber

The public helper is dependency-free Python 3 and installs only
`<host>/mods/TrinityCore`:

```powershell
python Trinity/tools/install_trinity.py --host-dir "X:\fefw-recomp-0.2.13" --version 0.2.0
python Trinity/tools/install_trinity.py --host-dir "X:\ObeliskChamber-0.3.2" --version 0.2.0
```

It downloads from the official GitHub release, verifies the archive against
`SHA256SUMS`, rejects absolute or traversal paths, and keeps an installed newer
version unless `--force` is passed. A local archive can be used for an offline
installation; the checksum file is still mandatory:

```powershell
python Trinity/tools/install_trinity.py --archive .\Trinity-v0.2.0-windows-x64.zip --checksums .\SHA256SUMS --mods-dir .\mods
```

The fefw-recomp 0.2.13 runtime is `NxModApi` 3, so it installs and runs the
plugin headless: settings, files and frame work, but no panel. ObeliskChamber
0.3.2 is API 5; the prepared runtime loaded the same payload, registered the
panel and reached the first frame headless, without drawing or interacting with
it. See [COMPATIBILITY.md](COMPATIBILITY.md) before claiming a panel or gameplay
result in either host.

## EmblemWeave

EmblemWeave builds that contain this integration first look for a drop-in
package beside `EmblemWeave.exe` or under `plugins/`. If one is present, it is
staged into the same runtime folder and takes precedence over the embedded
release. Otherwise the launcher embeds the Windows payload. On first start it
writes:

```text
%APPDATA%\EmblemWeave\play\mods\TrinityCore\exefs\Trinity.dll
%APPDATA%\EmblemWeave\play\mods\TrinityCore\plugin.json
%APPDATA%\EmblemWeave\play\mods\TrinityCore\emblemweave.json
```

A portable copy uses `data\play\mods\TrinityCore` instead. The version in
`plugin.json` is compared with the embedded version; an older install or a
missing payload is replaced, an equal or newer install is kept, and
`.nxp-settings.ini` is never removed. The existing runtime DLL loader then
starts it on Play. No other mod folder, hook site or loader path is changed.

Manual installation is the same folder copied under `data/play/mods`. Use the
canonical `Trinity.dll` name; Windows is case-insensitive, but release packages
must not rely on that. A drop-in DLL is version-checked the same way: an older
package does not replace a newer installed copy, and the settings file is left
alone.

## Bundled distribution for host maintainers

Host releases that must work offline can vendor the verified payload:

```bash
python Trinity/tools/install_trinity.py --version 0.2.0 --platform windows-x64 \
  --bundle vendor/trinity
```

This verifies the official archive and stages only `TrinityCore/`; it does not
write to a host profile. Copy that folder into the host's bundled mods location
(for example `mods/` for fefw-recomp or ObeliskChamber, or `data/play/mods/`
for EmblemWeave). At first run, compare `plugin.json` with the installed copy,
write the payload if the installed copy is older or absent, and keep the user's
settings. Pin the release version and checksum in the host build; do not fetch a
floating `latest` during startup.

## Linux

The Linux archive is a native x86-64 shared object, not a renamed Windows DLL.
A native host loads it with `dlopen` and resolves `nxp_get_plugin`; the NXMod
compatibility adapter also exports `nxmod_init` for an ABI-compatible loader.
There is no Linux GUI game launcher in this workspace, so the verified Linux
level is build, ELF/export inspection and a native `dlopen` harness, not
gameplay. Lua/Python hosts are not supported by this binary.

## Known limits

- The Windows plugin was loaded by the installed EmblemWeave 0.1.43 runtime at
  `E:\EmblemWeave` and the prepared fefw-recomp 0.2.13 runtime with `Quicksave`
  1.0.0 and `Turn Control` 0.3.0; that proves loading and initialization, not
  gameplay. The EmblemWeave payload was supplied as a drop-in beside the
  launcher and staged by the launcher, not embedded at build time.
- Those two mods target `NxModApi` directly. They do not load through Trinity's
  NXP adapter; they coexist with Trinity under the same host loader.
- The fefw-recomp run was headless on API 3, so Trinity had no panel and no
  save, turn or facility action was exercised. Quicksave reported that it could
  not find the save folder in that profile. See
  [evidence/fefw-runtime.log](evidence/fefw-runtime.log).
- An ObeliskChamber 0.3.2 game runtime was prepared for a headless run. The
  same binary loaded on API 5 with Quicksave and Turn Control, and Trinity
  registered its panel; the panel was not drawn and no gameplay was exercised.
  See [evidence/obelisk-runtime.log](evidence/obelisk-runtime.log).
- The Trinity panel registers, but a windowed capture of the panel is still
  pending.
- No gameplay hooks, guest memory access or save behavior are added by Trinity.
