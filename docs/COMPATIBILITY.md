# Compatibility matrix

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
| `add_panel` | Present, S and R | Absent in API 3 package, P | Present, P |
| `panel_visible` | Present, S and R | Absent in package strings, P | Absent from package strings; exact table not verified |
| `open_list` slot collision | No | No | Yes, API 5; runtime distinguishes by first argument |
| NXP compatibility adapter | Loaded, R and V | Not loaded, N | Not loaded, N |
| Native NXP adapter | Not implemented | Not implemented | Not implemented |
| Panel in NXP | Registered; the older demo panel was rendered, the Trinity 0.2.0 panel capture is pending | Unavailable on API 3 | Capability exists in source lineage, not run |
| Settings persistence through NXP | Checkbox and slider changed file, R | Not run, N | Not run, N |
| Launcher adapter source | Native page in the host tree | Tauri page source in this repo | Tauri page source in this repo |
| Launcher window verified | Native page captured from the release launcher | Actual 0.2.13 Tauri window built from the tag | Actual 0.3.2 Tauri window built from current source |
| Gameplay effect | None; Trinity changes no game state | None | None |

## 2a. Published code mods alongside Trinity

The original GameBanana DLLs were tested, not recompiled copies. Both target
`NxModApi` directly; neither is an NXP client, so this is coexistence with the
same host loader, not a test through Trinity's compatibility adapter.

| Mod | Original binary SHA-256 | Result on the development runtime | Not tested |
| --- | --- | --- | --- |
| [Quick Save 1.0.0](https://gamebanana.com/mods/742827) | `fec36b1da9ae0ce64c59c3e92e0ff6a970189d9d44b6a15161500016b3c0f91e` | Loaded; `[Quick] ready (mod API 4, Mods window)`; found its save folder | Pressing save/load or writing a save |
| [Turn Control 0.3.0](https://gamebanana.com/mods/745602) | `a9aabf02dc56b53afeda57646d90ff352c4172063ae7d4f01f9c5faacff258c0` | Loaded; `[Turns] ready (mod API 4, Mods window F8 + hotkeys)` | Changing turns, freeze or facilities |
| Trinity Core 0.2.0 | `f873124ed90671c961decda9bcd2678269de47ec614379044e89a22e9bd81cfd` | Loaded alongside both; panel registered; 5 prefixes accepted, 1 rejected by design | Gameplay effect (none exists) |

Evidence: [mod-compat-runtime.log](evidence/mod-compat-runtime.log). The game build
was `DB93B0125F4850601D61C3EDDE012F8EB2ECFE77` (version 131072). All three mods
loaded at the first frame and reached their own ready messages; no init declined.

## 2b. Platform matrix

| Platform | Build | Load | Runtime observation |
| --- | --- | --- | --- |
| Windows x64 | C | L: EmblemWeave development runtime | R: init with three mods, panel registration, prefix check |
| Linux x64 | C | L: native `dlopen` harness | R: init, panel, settings, frame and shutdown; no game runtime |

The Linux shared object is ELF64 x86-64 with `nxp_get_plugin` and `nxmod_init`.
It links only the standard C++/system libraries. This is not a Linux game host
result and does not prove a Linux launcher.

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
| Quick Save 1.0.0 and Turn Control 0.3.0 load and initialize alongside Trinity | L, R | `docs/evidence/mod-compat-runtime.log` |
| EmblemWeave first-run bundle installs, preserves settings and refuses a downgrade | C, R | 3 `trinity_bundle` tests passed |
| Trinity Core settings file is created | R | `trinity.enabled=1`, `trinity.prefix_guard=1` in the plugin folder |
| Demo panel renders in F1 > Advanced > Mods | V | `docs/evidence/emblemweave-mod-panel.png` |
| Trinity Core panel renders in F1 > Advanced > Mods | N | Panel registration is logged; a windowed capture is pending |
| Checkbox and slider persist through the adapter | R | Settings file read back, recorded in evidence README |
| Same DLL loads in fefw-recomp | N | No prepared runtime |
| Same DLL loads in ObeliskChamber | N | No prepared runtime |
| Launcher page builds in both upstream Tauri sources | C | Vite and Tauri production builds, 25 November 2026 |
| Trinity page renders in both launcher windows | V | Actual 0.2.13 and 0.3.2 Tauri captures in `docs/evidence/` |
| Launcher navigation opens the Trinity page | V | Actual window captures, 25 November 2026 |
| Launcher change is merged into either host repository | N | Local checkouts only |
| Packaged Tauri window | V | `launcher-fefw-trinity.png`, `launcher-obelisk-trinity.png` |
| Native EmblemWeave page | V | `emblemweave-trinity.png`, EmblemWeave 0.1.43 |
| Linux `.so` builds, exports both entry points and loads | C, L, R | ELF/readelf/nm output; CTest 5/5; native harness PASS |
| Gameplay changes | N | No gameplay interface |

## 6. Unknowns that matter

### EmblemWeave installed versus development executable

The installed `%APPDATA%\EmblemWeave\play\fefw.exe` inspected earlier is 220,880,384 bytes and lacks `nxmod_init`. The development executable at `engine/fefw-recomp/game-build/fefw/build/release/fefw.exe` supports code mods. The `X:\EmblemWeave` launcher was rebuilt for the native page capture; its packaged game runtime is unchanged. A release of EmblemWeave must include the newer runtime before players can use any code plugin.

### fefw-recomp API 4

The packaged 0.2.13 is API 3 and has no panel path. Issue #165 says API 4 is planned. A native adapter on API 4 should expose `nxp.ui`; on API 3 it must not.

### ObeliskChamber version 5

The API 5 table places `open_list` where the other ports place `add_panel`, and the runtime distinguishes the call by the first argument. The adapter calls `add_panel` with a title, which is the non-`Ctx` path. That is source-consistent but not runtime-verified in the packaged host. A native adapter should expose panels through `nxp.ui` and the list screen through an explicit `obeliskchamber.list` interface rather than relying on a version number.

### Single Windows binary

One Windows x64 plugin built against this header and the compatibility adapter loaded in one EmblemWeave development build alongside two published mods. That does not prove it loads in the other two packaged runtimes. The common ABI makes it plausible; the matrix records it as unproven.

### Linux is not a Linux game result

The ELF shared object is native and the native harness exercises the full NXP
lifecycle. There is no Linux game launcher or Linux game runtime in this
workspace, so the Linux claim must not be raised to gameplay or a launcher.

## 7. Next verification steps

1. Load `Trinity.dll` in prepared fefw-recomp and ObeliskChamber game runtimes; record the exact API table and host id.
2. Capture the Trinity panel in a windowed EmblemWeave run; registration is not a visual check.
3. Run one native Linux host or game runtime against `libTrinity.so`; the harness is not a game.
4. Add a native EmblemWeave adapter that calls `nxp_get_plugin` directly.
5. Exercise a Quick Save and a Turn Control action in gameplay after backing up saves; do not infer it from initialization.

## 8. Reporting rule

A future update must append evidence, not replace a lower level with a higher one without a test. "Source integrated" is not "runtime verified". "Panel rendered" is not "gameplay works". The matrix is the public claim.
