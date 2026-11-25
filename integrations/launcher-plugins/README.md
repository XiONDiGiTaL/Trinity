# Drop-in launcher discovery

Status: verified 25 November 2026 on EmblemWeave 0.1.43, fefw-recomp 0.2.13
and ObeliskChamber 0.3.2.

A player can put `Trinity.dll` beside a launcher executable, or under a
`plugins/` folder at the same root, and the launcher will stage it into the
game runtime's existing mods folder. The launcher never loads the DLL. The game
runtime does, through its normal code-mod loader, so no loader change, API bump
or second plugin path is required.

The prepared implementation is [`plugin_discovery.rs`](plugin_discovery.rs). It
is dependency-free Rust (standard library only) and is shared by the two Tauri
launchers. EmblemWeave has the same behaviour in its native
`apps/emblemweave/src/trinity_bundle.rs`, which prefers an external DLL over its
embedded payload.

## Why staging is necessary

Trinity is a runtime code mod. The launcher process cannot initialise it: the
plugin needs the game's host table, memory and frame loop. The existing runtime
loader already scans:

| Host | Runtime mods folder | Staged payload |
| --- | --- | --- |
| EmblemWeave | `data/play/mods` (portable) or `%APPDATA%\EmblemWeave\play\mods` | `TrinityCore/exefs/Trinity.dll` |
| fefw-recomp | `<install>/mods` | `TrinityCore/exefs/Trinity.dll` |
| ObeliskChamber | `<install>/mods` | `TrinityCore/exefs/Trinity.dll` |

The launcher's job is only to recognise the drop-in package and keep that
folder current. This keeps all three hosts on their existing loader and keeps
the plugin independent of the launcher UI.

## Recognised layouts

The first match wins:

| Priority | Path relative to the launcher executable |
| --- | --- |
| 1 | `plugins/TrinityCore/exefs/Trinity.dll` |
| 2 | `plugins/TrinityCore/lib/Trinity.dll` |
| 3 | `plugins/TrinityCore/Trinity.dll` |
| 4 | `plugins/Trinity.dll` |
| 5 | `TrinityCore/exefs/Trinity.dll` |
| 6 | `TrinityCore/lib/Trinity.dll` |
| 7 | `TrinityCore/Trinity.dll` |
| 8 | `Trinity.dll` |

On Linux the payload name is `libTrinity.so` in the same `exefs/`, `lib/` and
package-root positions. The `exefs/` spelling is kept because it is the existing
mod-package layout used by both runtimes; it does not mean the DLL is a game
executable.

A package folder may carry `plugin.json` at its root:

```text
plugins/
  TrinityCore/
    plugin.json
    exefs/
      Trinity.dll
```

A bare DLL is also accepted. When no manifest is supplied and no manifest is
already installed, the launcher writes a minimal manifest so the package is
self-describing. When a manifest is supplied, it is copied beside the staged
DLL.

## Add it to a Tauri launcher

1. Copy `plugin_discovery.rs` into `src-tauri/src/`.
2. Declare it in `src-tauri/src/main.rs`:

```rust
mod plugin_discovery;
```

3. Stage once during setup, after the window or app state exists:

```rust
.setup(|_app| {
    let root = exe_dir();
    if let Err(error) = plugin_discovery::stage(&root, &root.join("mods")) {
        eprintln!("[trinity] {error}");
    }
    Ok(())
})
```

4. Stage again before Play, in case the player configured a different run
   folder after startup:

```rust
plugin_discovery::stage(&exe_dir(), &p.mods())?;
```

Use the host's own mods helper. In fefw-recomp and ObeliskChamber that is
`p.mods()`; if a host has no helper, `p.game_dir.join("mods")` is the same path
used by the runtime.

A staging failure must not stop the launcher from opening. Record the error and
let the existing runtime report a missing plugin if the player tries to use it.

## EmblemWeave

EmblemWeave already calls `trinity_bundle::ensure_installed()` at startup. The
external lookup is checked first, using `crate::platform::exe_dir()`:

```rust
if let Some(dll) = external_dll() {
    return ensure_external_in(&mods, &dll);
}
ensure_in(&mods) // embedded release fallback
```

This means an installed `Trinity.dll` wins over the bundled copy, while a
missing external package still falls back to the embedded release. The staged
destination remains `data/play/mods/TrinityCore`.

## Version and settings rules

- Compare `plugin.json` versions, not timestamps.
- A newer installed version is kept when an older external package appears.
- An equal or newer external package replaces the staged DLL and manifest.
- A bare DLL with no manifest is staged because putting it in the launcher root
  is an explicit request to use it.
- Never delete or rewrite `.nxp-settings.ini`; the module only creates the
  `TrinityCore` folder and writes the DLL, manifest and host metadata.
- Use an atomic copy through a temporary file so a failed write does not leave a
  truncated DLL.

The version guard is intentionally conservative. A player who deliberately
drops a newer DLL into the launcher root expects it to replace an older staged
copy; a player who still has a newer managed install should not be downgraded by
an old package left beside the launcher.

## Verification performed

EmblemWeave at `E:\EmblemWeave`, fefw-recomp and ObeliskChamber were each run
with `Trinity.dll` in the root and under `plugins/` (flat and nested package
forms). The second EmblemWeave install at `X:\EmblemWeave` was run with the root
DLL. Before each run the staged DLL was replaced with a stale marker. All three
launchers restored the real payload:

| Launcher | Drop-in location tested | Staged result |
| --- | --- | --- |
| EmblemWeave 0.1.43 at `E:\EmblemWeave` | `Trinity.dll`; `plugins\Trinity.dll` | `data\play\mods\TrinityCore\exefs\Trinity.dll` |
| fefw-recomp 0.2.13 at `X:\fefw-recomp-0.2.13` | root DLL; `plugins\TrinityCore\exefs\Trinity.dll` | `mods\TrinityCore\exefs\Trinity.dll` |
| ObeliskChamber 0.3.2 at `X:\ObeliskChamber-0.3.2` | root DLL; `plugins\Trinity.dll` | `mods\TrinityCore\exefs\Trinity.dll` |
| EmblemWeave 0.1.43 at `X:\EmblemWeave` | root `Trinity.dll` | `data\play\mods\TrinityCore\exefs\Trinity.dll` |

Every staged copy matched SHA-256
`f873124ed90671c961decda9bcd2678269de47ec614379044e89a22e9bd81cfd` and
`plugin.json` version `0.2.0`. Each runtime then loaded the staged plugin and
logged its host identity:

```text
mods: TrinityCore/Trinity.dll: Trinity Core 0.2.0 on emblemweave (nxmod-api-4)
mods: TrinityCore/Trinity.dll: Trinity Core 0.2.0 on fefw-recomp (nxmod-api-3)
mods: TrinityCore/Trinity.dll: Trinity Core 0.2.0 on obelisk-chamber (nxmod-api-5)
mods: TrinityCore/Trinity.dll: Trinity Core 0.2.0 on emblemweave (nxmod-api-4)  # X:\EmblemWeave
```

The `X:\EmblemWeave` runtime had no game INI; a valid one was generated from
the E configuration, reusing the E dump and pointing mods/saves/work at X. Its
shader cache was empty, so translated draws were skipped; the load result is
valid, but it is not a visual result.

The runs were headless. They prove discovery, staging, loading and
initialization, not panel drawing or gameplay. Full excerpts are in
[`docs/evidence/`](../../docs/evidence/README.md).

## Prepared patches

The local host checkouts contain other Trinity page work as well as this module.
These patches isolate only the drop-in discovery change:

| Host | Patch |
| --- | --- |
| EmblemWeave | [`../patches/emblemweave-plugin-discovery.patch`](../patches/emblemweave-plugin-discovery.patch) |
| fefw-recomp | [`../patches/fefw-launcher-plugin-discovery.patch`](../patches/fefw-launcher-plugin-discovery.patch) |
| ObeliskChamber | [`../patches/obelisk-launcher-plugin-discovery.patch`](../patches/obelisk-launcher-plugin-discovery.patch) |

Apply a Tauri patch from the repository that contains its `launcher/` folder:

```bash
git apply --check path/to/fefw-launcher-plugin-discovery.patch
git apply path/to/fefw-launcher-plugin-discovery.patch
```

The EmblemWeave patch applies from the EmblemWeave repository root. The patches
add the module and the two call sites; they do not change global styling,
existing pages, dependencies or the mod manifest format.
