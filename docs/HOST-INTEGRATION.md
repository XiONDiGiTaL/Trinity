# Launcher integration guide

This guide is for the maintainers of EmblemWeave, fefw-recomp, ObeliskChamber and other hosts that want to consume Trinity. It covers the released binaries, not building Trinity from source. A launcher project does not need CMake, a compiler or a Trinity checkout to ship the plugin.

## 1. What the launcher owns

Trinity keeps the launcher and the game runtime in separate processes:

- The **launcher** reads `plugin.json`, validates identity and versions, installs or updates the plugin folder, shows compatibility, and never loads the plugin binary.
- The **runtime** loads the native module, validates the NXP descriptor, builds the host table, and calls `init`.

The launcher keeps its existing pages, navigation, visual language and update path. Trinity adds one code-mod entry or one plugin folder; it does not replace the launcher shell.

## 2. Release layout

| Platform | Archive | Payload |
| --- | --- | --- |
| Windows x64 | `Trinity-v<version>-windows-x64.zip` | `TrinityCore/exefs/Trinity.dll` |
| Linux x64 | `Trinity-v<version>-linux-x64.tar.gz` | `TrinityCore/lib/libTrinity.so` and `include/nxp/nxp.h` |

Every release also publishes `SHA256SUMS` and `release.json`. `plugin.json` inside `TrinityCore` carries the plugin id, semantic version, NXP ABI range, supported host ids and entry paths.

The archive format is fixed. Do not rename `Trinity.dll` to `libTrinity.so` or the reverse; they are different binary formats for different loaders.

## 3. Download and verify an official release

Release URL shape:

```text
https://github.com/XiONDiGiTaL/Trinity/releases/download/v<version>/
  Trinity-v<version>-windows-x64.zip
  Trinity-v<version>-linux-x64.tar.gz
  SHA256SUMS
  release.json
```

Recommended order:

1. Fetch `SHA256SUMS` first.
2. Download the archive for the host platform.
3. Compute SHA-256 and compare it with the named entry before extracting anything.
4. Reject a missing entry, a malformed digest or a mismatch.
5. Extract into a staging directory, then move the payload into place.

`tools/install_trinity.py` implements this order without third-party Python packages:

```powershell
python tools/install_trinity.py --host-dir "C:\path\to\host" --version 0.2.0
```

It refuses traversal and absolute paths, installs only `TrinityCore`, keeps an installed equal or newer version unless `--force` is passed, and preserves `.nxp-settings.ini`. Use `--platform linux-x64` for a Linux payload.

## 4. Bundle a payload for offline releases

A host release that must work offline should vendor the verified payload instead of downloading at startup:

```bash
python tools/install_trinity.py --version 0.2.0 --platform windows-x64 --bundle vendor/trinity
```

Copy the staged `vendor/trinity/TrinityCore` into the host's bundled mods location, pin the release version and checksum in the host build, and copy it to the player's mods folder on first run when the installed copy is absent or older. Do not fetch a floating `latest` during startup, and do not delete the player's settings file during an update.

## 5. Where the plugin lives at runtime

| Host | Default mods folder | Installed folder |
| --- | --- | --- |
| EmblemWeave | `%APPDATA%\EmblemWeave\play\mods` or portable `data\play\mods` | `TrinityCore\exefs\Trinity.dll` (Windows) |
| fefw-recomp | `<host>\mods` | `TrinityCore\exefs\Trinity.dll` (Windows) |
| ObeliskChamber | `<host>\mods` | `TrinityCore\exefs\Trinity.dll` (Windows) |
| Any Linux host | host-defined mods folder | `TrinityCore/lib/libTrinity.so` |

A folder whose name begins with `_` or `-` is disabled by the shared runtime lineage. Use that existing convention rather than inventing a second enabled flag.

### 5a. Drop-in discovery beside the launcher

A launcher can also recognise a package that the player placed beside its own executable, without using the installer or an embedded payload. The launcher stages the payload into the runtime mods folder above; it still never loads the DLL itself.

The prepared implementation is [integrations/launcher-plugins/plugin_discovery.rs](../integrations/launcher-plugins/plugin_discovery.rs), with adaptation notes in [integrations/launcher-plugins/README.md](../integrations/launcher-plugins/README.md). It is standard-library-only Rust, so a host can copy one file into `src-tauri/src/` and add `mod plugin_discovery;`.

Search order, first match wins:

| Priority | Path relative to the launcher |
| --- | --- |
| 1 | `plugins/TrinityCore/exefs/Trinity.dll` |
| 2 | `plugins/TrinityCore/lib/Trinity.dll` |
| 3 | `plugins/TrinityCore/Trinity.dll` |
| 4 | `plugins/Trinity.dll` |
| 5 | `TrinityCore/exefs/Trinity.dll` |
| 6 | `TrinityCore/lib/Trinity.dll` |
| 7 | `TrinityCore/Trinity.dll` |
| 8 | `Trinity.dll` |

On Linux, `libTrinity.so` takes the DLL's place. A package may include `plugin.json` beside the payload; a bare DLL is still staged, because placing it beside the launcher is an explicit request to use it.

For a Tauri launcher, stage during setup and again before Play:

```rust
mod plugin_discovery;

.setup(|_app| {
    let root = exe_dir();
    if let Err(error) = plugin_discovery::stage(&root, &root.join("mods")) {
        eprintln!("[trinity] {error}");
    }
    Ok(())
})
```

```rust
plugin_discovery::stage(&exe_dir(), &p.mods())?;
```

EmblemWeave already checks for an external DLL before its embedded payload in `trinity_bundle::ensure_installed()`. The external lookup uses the launcher's own `exe_dir()`, then stages to `data/play/mods/TrinityCore`.

The staging rule is version-aware but not surprising: a package with a manifest newer than the installed copy replaces it; an older package leaves a newer install alone; a bare DLL is staged and receives a minimal manifest if none is installed. `.nxp-settings.ini` is never touched, and the copy goes through a temporary file so a failed write cannot leave a truncated payload.

This path was verified on 25 November 2026 against EmblemWeave 0.1.43 at `E:\EmblemWeave` and `X:\EmblemWeave`, fefw-recomp 0.2.13 at `X:\fefw-recomp-0.2.13` and ObeliskChamber 0.3.2 at `X:\ObeliskChamber-0.3.2`. Root and `plugins/` placements (flat and nested package) all restored hash `f873124e...bd81cfd` into the host's mods folder, and each runtime then logged `Trinity Core 0.2.0 on <host>`. The X:\EmblemWeave runtime needed a generated INI that reused the E: game dump; its empty shader cache skipped translated draws, so that run is a load result, not a visual one.

## 6. Loading and initialization

Two entry points are possible:

- **Legacy `NxModApi` host:** the existing loader calls `nxmod_init(const NxModApi* api, const char* mod_dir)` from the compatibility adapter. No host API version bump is required.
- **Native NXP host:** the loader resolves `nxp_get_plugin(uint32_t host_abi_version)`, validates the descriptor and calls `init(const NxpHostApi* host)`.

A native adapter should follow the rules in [NXP.md](NXP.md): validate `abi_version`, `size` and `init` before calling anything; return only implemented interfaces; drive panel callbacks on the presenting thread and frame callbacks on the game main thread; keep `nxmod_init` working for legacy mods.

On a host with no panel slot (for example `NxModApi` 3), the adapter exposes settings, files and frame only. Trinity Core detects this, logs that it is running headless, and continues. That is a supported state, not a failed load.

## 7. Version and compatibility checks

Before enabling the entry, check:

| Check | Failure behavior |
| --- | --- |
| `plugin.json` exists and parses | Show "Trinity is installed but its manifest is missing or invalid"; keep it disabled. |
| `plugin.json` version is newer than or equal to the bundled minimum | Offer the update; do not silently downgrade a newer install. |
| `abi.min <= NXP_ABI_VERSION <= abi.max` | Disable with the exact ABI range in the message. |
| `hosts` includes this host id (`emblemweave`, `fefw-recomp`, `obelisk-chamber`) | Disable and name the supported hosts. |
| Entry file exists for this platform | Disable and name the expected artifact (`Trinity.dll` or `libTrinity.so`). |
| The runtime's `NxModApi`/NXP table is at least the size the adapter expects | Let the adapter expose only the available interfaces rather than guessing. |

The NXP descriptor remains authoritative at runtime. A manifest is package metadata, not proof that the module exports the expected entry point.

## 8. Updates

- Keep `.nxp-settings.ini` and any player-added files; replace only payload files.
- Stage the new folder, then replace the old one. Do not leave a half-written `TrinityCore`.
- Compare semantic versions, not file timestamps.
- The loader does not unload a module. Apply an update on the next launch and say so in the UI.
- Keep the previous payload until the new copy is complete so a failed update can be retried.
- Check `SHA256SUMS` for downloaded updates even when the archive URL is pinned.

## 9. Error reporting

Useful messages name the missing condition and the next action:

| Condition | Suggested message |
| --- | --- |
| Plugin folder absent | "Trinity is not installed. Install it from the Mods page." |
| Manifest invalid | "Trinity's plugin.json is invalid; reinstall the release." |
| ABI mismatch | "Trinity 0.2.0 expects NXP ABI 1; this launcher provides ABI N." |
| Host not supported | "Trinity 0.2.0 supports EmblemWeave, fefw-recomp and ObeliskChamber; this host is <id>." |
| Wrong platform artifact | "This Trinity package is for Windows x64; this launcher needs libTrinity.so." |
| Load declined | Show the runtime's own log line; do not reduce it to "mod failed". |

Keep the launcher alive when a plugin is missing or incompatible. A disabled entry with a reason is better than a failed startup.

## 10. Preserve the host interface

The Tauri adapters in `hosts/launchers/` add one page module, one `PAGES` entry, one navigation label and a CSS import. The EmblemWeave adapter adds one page variant and one navigation group in the host's own egui tree. Neither replaces an existing page or changes the host's design tokens.

The shared page reads `shared/catalog.json` and `shared/prefixes.json`. Treat them as read-only inputs. If a host embeds a snapshot, refresh it with `integrations/sync-trinity.ps1` and rebuild; do not edit the snapshot by hand.

## 11. Verification checklist

Use the same evidence levels as [COMPATIBILITY.md](COMPATIBILITY.md):

1. **Source:** the adapter compiles against the host's public headers.
2. **Build:** the launcher and plugin artifacts are produced.
3. **Load:** the runtime logs that `Trinity.dll` or `libTrinity.so` loaded.
4. **Init:** the plugin reports the true host id, API version and capabilities.
5. **Registration:** panel, settings or frame interfaces register where the API supports them.
6. **Observed:** the panel is drawn, a setting persists, or the frame callback runs.
7. **Gameplay:** a save, turn or other game action is exercised. Trinity does not currently change game state; this step is for the mod being tested.

For a drop-in package, add a discovery check before load: put the payload beside the launcher and under `plugins/`, launch the host, and confirm the staged `TrinityCore/exefs/Trinity.dll` matches the supplied hash. A launcher-level copy alone is not a load result; the runtime log must still show `Trinity Core 0.2.0 on <host>`.

Do not promote a host's status on a successful build alone. Record the exact host version, API version, artifact hash and log line.

## 12. Current host notes

- **EmblemWeave:** API 4; `add_panel` and `panel_visible` are present. The installed 0.1.43 runtimes at `E:\EmblemWeave` and `X:\EmblemWeave` loaded the drop-in 0.2.0 payload, registered the Trinity panel and logged the host identity. The X runtime required a generated INI; its empty shader cache skipped translated draws.
- **fefw-recomp 0.2.13:** API 3; `add_panel` is absent. The plugin runs headless with settings, files and frame. API 4 is planned upstream and should expose the panel when it arrives.
- **ObeliskChamber 0.3.2:** API 5; the packaged table's `open_list` slot overlaps the other ports' `add_panel`. The adapter's panel registration was observed headless. A native adapter should expose the list screen as an explicit `obeliskchamber.list` interface rather than infer it from the version.

The full evidence ledger and unresolved questions are in [COMPATIBILITY.md](COMPATIBILITY.md).
