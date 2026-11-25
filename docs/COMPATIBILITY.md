# Compatibility matrix

Status: 25 November 2026. The public claims in the README and the launcher catalog must match this file.

## 1. Evidence levels

Every claim uses one of these labels:

| Label | Meaning |
| --- | --- |
| S | Read from source code, public headers or repository files |
| P | Read from a packaged binary by file inspection or string search |
| C | Compiled successfully in this workspace |
| L | Loaded by a host and reported as loaded |
| R | Observed at runtime through logs, settings or UI |
| V | Visually inspected in a host window |
| N | Not tested, or not testable with the available packages |

Compilation is not loading. Loading is not rendering. Rendering is not gameplay.

## 2. Host matrix

| Area | EmblemWeave | fefw-recomp 0.2.13 | ObeliskChamber 0.3.2 |
| --- | --- | --- | --- |
| Source inspected | S, full checkout at `c302f0a` | S, v0.2.13 tag and current master | S, current 0.3.2 source |
| Package inspected | S, development build | P | P |
| `NXMOD_API_VERSION` | 4 | 3 in package; 4 planned upstream | 5 |
| `nxmod_init` | Present, S and L | Present, P | Present, P |
| `nxmod_early` | Present, S | Present, P | Present, P |
| `on_frame` | Present, S and R | Present, P | Present, P |
| `add_panel` | Present, S and R | Absent in API 3 package, P | Present, P and R |
| `panel_visible` | Present, S and R | Absent in package strings, P | Not confirmed in package strings; the panel registration path was observed |
| `open_list` slot collision | No | No | Yes, API 5; runtime distinguishes by first argument |
| NXP compatibility adapter | Loaded, R and V | Loaded, R | Loaded, R |
| Native NXP adapter | Not implemented | Not implemented | Not implemented |
| Panel in NXP | Registered; the demo panel was rendered, the Trinity 0.2.0 panel capture is pending | Unavailable on API 3 | Registered in a headless run; not drawn or interacted with |
| Settings persistence through NXP | Checkbox and slider changed file, R | Settings file created; no UI on API 3, R | Not exercised, N |
| Launcher adapter source | Native page in the host tree | Tauri page source in this repo | Tauri page source in this repo |
| Launcher window verified | Native page captured from the release launcher | Actual 0.2.13 Tauri window built from the tag | Actual 0.3.2 Tauri window built from current source |
| Gameplay effect | None; Trinity changes no game state | None | None |

## 2a. Published code mods alongside Trinity

The original GameBanana DLLs were tested, not recompiled copies. Both target
`NxModApi` directly; neither is an NXP client, so this is coexistence with the
same host loader, not a test through Trinity's compatibility adapter.

The GameBanana IDs are easy to swap: `742827` is Turn Control and `745602` is
Quicksave. The hashes below were checked against the installed DLLs in both
prepared hosts.

| Mod | Original binary SHA-256 | EmblemWeave API 4 | fefw-recomp API 3 | ObeliskChamber API 5 |
| --- | --- | --- | --- | --- |
| [Turn Control 0.3.0](https://gamebanana.com/mods/742827) | `a9aabf02dc56b53afeda57646d90ff352c4172063ae7d4f01f9c5faacff258c0` | Loaded; `[Turns] ready (mod API 4, Mods window F8 + hotkeys)`; no turn action | Loaded; `[Turns] ready (mod API 3, System menu + hotkeys)`; no turn action | Loaded; `[Turns] ready (mod API 5, Obelisk Chamber menu + hotkeys)`; no turn action |
| [Quicksave 1.0.0](https://gamebanana.com/mods/745602) | `fec36b1da9ae0ce64c59c3e92e0ff6a970189d9d44b6a15161500016b3c0f91e` | Loaded; `[Quick] ready (mod API 4, Mods window)`; found its save folder; no save/load action | Loaded; `[Quick] ready (mod API 3)`; save folder not found in the headless profile; no save/load action | Loaded; `[Quick] ready (mod API 5)`; save folder not found in the headless profile; no save/load action |
| Trinity Core 0.2.0 | `f873124ed90671c961decda9bcd2678269de47ec614379044e89a22e9bd81cfd` | Loaded; panel registered; 5 prefixes accepted, 1 rejected by design | Loaded headless; no UI on API 3; 5 prefixes accepted, 1 rejected | Loaded; panel registered; 5 prefixes accepted, 1 rejected |

Evidence: [mod-compat-runtime.log](evidence/mod-compat-runtime.log),
[fefw-runtime.log](evidence/fefw-runtime.log) and
[obelisk-runtime.log](evidence/obelisk-runtime.log). The game build was
`DB93B0125F4850601D61C3EDDE012F8EB2ECFE77` (version 131072).

The EmblemWeave run was headless load/init: all three mods reached their own
ready messages and no init declined. Quicksave found its save folder there, but
no save or load was exercised.

The fefw-recomp 0.2.13 run used the same original DLLs on API 3. Quicksave and
Turn Control reported `ready (mod API 3)`; Trinity reported `0.2.0 on
fefw-recomp (nxmod-api-3)` and stayed headless because that table has no
`nxp.ui`. Quicksave reported that its save folder was not found in the headless
profile. Turn Control registered its System submenu. No save, turn, facility or
panel action was performed.

The ObeliskChamber 0.3.2 run used the same original DLLs on API 5. Trinity
reported `0.2.0 on obelisk-chamber (nxmod-api-5)`, accepted the same prefix set
and registered the panel; Turn Control registered its list screen; Quicksave
reported `ready (mod API 5)` but no save folder. The run reached the first
frame. It was headless, so nothing was drawn and no save, turn, facility or
panel action was performed.

## 2b. Platform matrix

| Platform | Build | Load | Runtime observation |
| --- | --- | --- | --- |
| Windows x64 | C | L: EmblemWeave development runtime, prepared fefw-recomp 0.2.13 and prepared ObeliskChamber 0.3.2 | R: init with three mods in all three hosts; first frame in fefw-recomp and ObeliskChamber; panel registration in EmblemWeave and ObeliskChamber; demo panel rendered in EmblemWeave; settings written |
| Linux x64 | C | L: native `dlopen` harness | R: init, panel, settings, frame and shutdown in the harness; no game runtime |

The Linux shared object is ELF64 x86-64 with `nxp_get_plugin` and `nxmod_init`.
It links only the standard C++/system libraries. This is not a Linux game host
result and does not prove a Linux launcher. Steam Deck and Proton were not
tested.

## 3. What the compatibility adapter exposes

The adapter is linked into a plugin and implements `nxmod_init`. It builds an `NxpHostApi` from the `NxModApi` the host passes.

| Host API | `nxp.ui` | `nxp.settings` | `nxp.files` | `nxp.frame` |
| --- | --- | --- | --- | --- |
| fefw-recomp API 3 | No | Yes | Yes | Yes |
| EmblemWeave API 4 | Yes | Yes | Yes | Yes |
| ObeliskChamber API 5 | Yes, through `add_panel` | Yes | Yes | Yes |

The adapter never invents an interface. A v3 host gets a headless plugin that can still use settings, files and frame work. A v4 or v5 host gets a panel if the table has `add_panel` and `panel_visible`.

The adapter detects `host_id` from the module path. A renamed executable can fall back to `nxmod-compatible`. Native adapters must pass the true id. Plugins must use capabilities for behaviour, not the path-derived name.

## 4. Launcher compatibility

The two Tauri launchers use a `PAGES` map. The Trinity adapter adds one page module and one navigation entry without changing the host's existing pages. The shared page reads `shared/catalog.json` and `shared/prefixes.json`.

EmblemWeave's launcher is egui/Rust. The native page lives in the host tree as a separate uncommitted change, and its release frame is captured in `docs/evidence/`. No other EmblemWeave behavior is changed by this repository.

The fefw-recomp v0.2.13 tag and the current ObeliskChamber 0.3.2 source were integrated with one Trinity page and built as Tauri release executables. Both were installed in their target folders and captured from their actual windows at 1180 x 1000. This verifies the page, host styling, catalog rendering and navigation in the real launcher process. The change has not been merged into either host repository.

### 4a. Drop-in discovery

Each launcher was rebuilt with the prepared discovery module. A copy of
`Trinity.dll` beside the launcher, under `plugins/`, or in a nested
`plugins/TrinityCore/exefs/` package is staged into the runtime's existing mods
folder. The launcher does not load the DLL. Before every run the staged payload
was replaced with a stale marker, so a matching hash after launch proves the
staging path, not a pre-existing file.

| Check | EmblemWeave 0.1.43 | fefw-recomp 0.2.13 | ObeliskChamber 0.3.2 |
| --- | --- | --- | --- |
| Root DLL recognised | V, `E:\EmblemWeave\Trinity.dll` and `X:\EmblemWeave\Trinity.dll` | V, root DLL | V, root DLL |
| `plugins/` DLL recognised | V, flat `plugins\Trinity.dll` | V, nested `plugins\TrinityCore\exefs\Trinity.dll` | V, flat `plugins\Trinity.dll` |
| Staged hash after launch | `f873124e...bd81cfd` | `f873124e...bd81cfd` | `f873124e...bd81cfd` |
| Manifest after staging | `0.2.0` | `0.2.0` | `0.2.0` |
| Runtime loaded after staging | L, `Trinity Core 0.2.0 on emblemweave (nxmod-api-4)` | L, `Trinity Core 0.2.0 on fefw-recomp (nxmod-api-3)` | L, `Trinity Core 0.2.0 on obelisk-chamber (nxmod-api-5)` |
| Settings preserved | Unit test; no settings file in the fresh install | Existing `.nxp-settings.ini` remained | Existing `.nxp-settings.ini` remained |

Evidence: [emblemweave-dropin-runtime.log](evidence/emblemweave-dropin-runtime.log),
[fefw-dropin-runtime.log](evidence/fefw-dropin-runtime.log) and
[obelisk-dropin-runtime.log](evidence/obelisk-dropin-runtime.log). The staging
source and adaptation notes are in
[integrations/launcher-plugins](../integrations/launcher-plugins/README.md).
The runtime runs were headless and ended at the `--max-seconds` deadline; they
prove staging, loading and initialization, not panel drawing or gameplay.

## 5. Verification ledger

| Claim | Level | Evidence |
| --- | --- | --- |
| NXP header compiles | C | Build output |
| Demo and Trinity Core can be built | C | `tools/build-windows.ps1`, 25 November 2026 |
| Host harness exercises discovery, panel, settings, frame and shutdown | C, R | CTest 5/5 passed on Windows and Linux |
| Compatibility adapter maps simulated API 3, 4 and 5 tables | C, R | `nxmod_adapter_api3`, `api4`, `api5` passed |
| Adapter settings and file callbacks round-trip through fake host | C, R | `nxmod_adapter_test` |
| Compatibility adapter links against `NxModApi` v4 | C | Build output |
| EmblemWeave development build loads `nxmod_init` | L | Runtime log in `docs/evidence/` |
| Trinity Core 0.2.0 loads, identifies the host and registers a panel in EmblemWeave | L | `docs/evidence/trinity-core-runtime.log`, `docs/evidence/mod-compat-runtime.log` |
| Trinity Core prefix classifier accepts five and rejects one unknown name at runtime | R | `docs/evidence/mod-compat-runtime.log` |
| Turn Control 0.3.0 and Quicksave 1.0.0 load and initialize alongside Trinity in EmblemWeave | L, R | `docs/evidence/mod-compat-runtime.log` |
| The same DLL loads in fefw-recomp 0.2.13 and runs headless | L, R | `docs/evidence/fefw-runtime.log` |
| The same DLL loads in ObeliskChamber 0.3.2 and registers its panel headless | L, R | `docs/evidence/obelisk-runtime.log` |
| Turn Control 0.3.0 and Quicksave 1.0.0 load and initialize alongside Trinity in fefw-recomp and ObeliskChamber | L, R | `docs/evidence/fefw-runtime.log`, `docs/evidence/obelisk-runtime.log` |
| EmblemWeave first-run bundle installs, preserves settings and refuses a downgrade | C, R | 3 `trinity_bundle` tests passed |
| Trinity Core settings file is created | R | `trinity.enabled=1`, `trinity.prefix_guard=1` in the plugin folder |
| Demo panel renders in F1 > Advanced > Mods | V | `docs/evidence/emblemweave-mod-panel.png` |
| Trinity Core panel renders in F1 > Advanced > Mods | N | Panel registration is logged; a windowed capture is pending |
| Trinity Core panel renders in ObeliskChamber | N | Panel registration is logged; the run was headless |
| Checkbox and slider persist through the adapter | R | Settings file read back, recorded in evidence README |
| Launcher page builds in both upstream Tauri sources | C | Vite and Tauri production builds, 25 November 2026 |
| Trinity page renders in both launcher windows | V | Actual 0.2.13 and 0.3.2 Tauri captures in `docs/evidence/` |
| Launcher navigation opens the Trinity page | V | Actual window captures, 25 November 2026 |
| Launcher recognises a root `Trinity.dll` and stages it | C, R | Root-dropin runs in all three installed launchers, 25 November 2026 |
| Launcher recognises `plugins/Trinity.dll` (flat and nested) and stages it | C, R | Plugins-dropin runs in all three installed launchers, 25 November 2026 |
| Staged drop-in loads in the game runtime | L | `emblemweave-dropin-runtime.log`, `fefw-dropin-runtime.log`, `obelisk-dropin-runtime.log` |
| Launcher change is merged into either host repository | N | Local checkouts only |
| Native EmblemWeave page | V | `emblemweave-trinity.png`, EmblemWeave 0.1.43 |
| Linux `.so` builds, exports both entry points and loads | C, L, R | ELF/readelf/nm output; CTest 5/5; native harness PASS |
| Steam Deck or Proton compatibility | N | Not tested |
| Gameplay changes | N | No gameplay interface |

## 6. Unknowns that matter

### EmblemWeave installed runtime

The installed EmblemWeave 0.1.43 runtime at `E:\EmblemWeave\data\play\fefw.exe`
loads code mods and was used for the drop-in run: it staged and loaded the 0.2.0
payload, identified `emblemweave` / `nxmod-api-4`, registered the panel and ran
the prefix classifier. The older packaged runtime at `X:\EmblemWeave` was not
used for this run and is not a load result. A future EmblemWeave release must
keep a runtime with `nxmod_init`; the launcher package alone is not the loader.

### fefw-recomp API 4

The packaged 0.2.13 is API 3 and has no panel path. Issue #165 says API 4 is planned. A native adapter on API 4 should expose `nxp.ui`; on API 3 it must not.

### ObeliskChamber version 5

The API 5 table places `open_list` where the other ports place `add_panel`, and the runtime distinguishes the call by the first argument. The adapter calls `add_panel` with a title, which is the non-`Ctx` path. The packaged 0.3.2 runtime loaded the adapter and logged `added the panel "Trinity" to the Mods window` and `panel registered`, so the loading and registration path is runtime-observed. The panel was not drawn in the headless run, and the exact packaged table has not been probed member by member. A native adapter should expose panels through `nxp.ui` and the list screen through an explicit `obeliskchamber.list` interface rather than relying on a version number.

### Single Windows binary

One Windows x64 plugin built against this header and the compatibility adapter loaded in the EmblemWeave development runtime, the prepared fefw-recomp 0.2.13 runtime and the prepared ObeliskChamber 0.3.2 runtime, alongside the same two published mods. That proves loading and initialization on three host implementations, not gameplay, and not a panel drawn in every host.

### Linux is not a Linux game result

The ELF shared object is native and the native harness exercises the full NXP
lifecycle. There is no Linux game launcher or Linux game runtime in this
workspace, so the Linux claim must not be raised to gameplay or a launcher.
Steam Deck and Proton were not tested.

## 7. Next verification steps

1. Capture the Trinity panel in a windowed EmblemWeave run and exercise its settings; registration is not a visual check.
2. Draw and interact with the Trinity panel in a windowed ObeliskChamber run.
3. Run one native Linux host or game runtime against `libTrinity.so`; the harness is not a game.
4. Add a native EmblemWeave adapter that calls `nxp_get_plugin` directly.
5. Exercise a Quicksave and a Turn Control action in gameplay after backing up saves; do not infer it from initialization.
6. Re-test the panel path when fefw-recomp ships API 4.

## 8. Reporting rule

A future update must append evidence, not replace a lower level with a higher one without a test. "Source integrated" is not "runtime verified". "Panel rendered" is not "gameplay works". The matrix is the public claim.
