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
| Source inspected | S, full checkout at `c302f0a` | S, upstream clone newer than package | S, upstream clone newer than package |
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
| Panel in NXP | Rendered in F1 > Advanced > Mods | Unavailable on API 3 | Capability exists in source lineage, not run |
| Settings persistence through NXP | Checkbox and slider changed file, R | Not run, N | Not run, N |
| Launcher adapter source | Design only | Tauri page source in this repo | Tauri page source in this repo |
| Launcher build verified | N/A | Not built in this repository | Not built in this repository |
| Gameplay effect | None; the demo changes no game state | None | None |

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

EmblemWeave's launcher is egui/Rust. It needs a native page and an additional navigation group. The existing tests require every page to belong to exactly one group. No EmblemWeave source has been changed in this repository.

No launcher build has been run from this repository yet. The status is source-level until a host checkout builds it.

## 5. Verification ledger

| Claim | Level | Evidence |
| --- | --- | --- |
| NXP header compiles | C | Build output |
| Demo and Trinity Core can be built | C | `tools/build-windows.ps1`, 25 November 2026 |
| Host harness exercises discovery, panel, settings, frame and shutdown | C, R | CTest 2/2 passed |
| Compatibility adapter links against `NxModApi` v4 | C | Build output |
| EmblemWeave development build loads `nxmod_init` | L | Runtime log in `docs/evidence/` |
| Trinity Core loads, identifies the host and registers a panel in EmblemWeave | L | `docs/evidence/trinity-core-runtime.log` |
| Trinity Core prefix classifier accepts five and rejects one unknown name at runtime | R | `docs/evidence/trinity-core-runtime.log` |
| Trinity Core settings file is created | R | `trinity.enabled=1`, `trinity.prefix_guard=1` in the plugin folder |
| Demo panel renders in F1 > Advanced > Mods | V | `docs/evidence/emblemweave-mod-panel.png` |
| Trinity Core panel renders in F1 > Advanced > Mods | N | Panel registration is logged; a windowed capture is pending |
| Checkbox and slider persist through the adapter | R | Settings file read back, recorded in evidence README |
| Same DLL loads in fefw-recomp | N | No prepared runtime |
| Same DLL loads in ObeliskChamber | N | No prepared runtime |
| Launcher page renders in either Tauri launcher | N | Source only |
| Native EmblemWeave page | N | Design only |
| Linux `.so` builds and loads | N | No Linux build |
| Gameplay changes | N | No gameplay interface |

## 6. Unknowns that matter

### EmblemWeave installed versus development executable

The installed `%APPDATA%\EmblemWeave\play\fefw.exe` inspected earlier is 220,880,384 bytes and lacks `nxmod_init`. The development executable at `engine/fefw-recomp/game-build/fefw/build/release/fefw.exe` supports code mods. A release of EmblemWeave must include the newer runtime before players can use any code plugin.

### fefw-recomp API 4

The packaged 0.2.13 is API 3 and has no panel path. Issue #165 says API 4 is planned. A native adapter on API 4 should expose `nxp.ui`; on API 3 it must not.

### ObeliskChamber version 5

The API 5 table places `open_list` where the other ports place `add_panel`, and the runtime distinguishes the call by the first argument. The adapter calls `add_panel` with a title, which is the non-`Ctx` path. That is source-consistent but not runtime-verified in the packaged host. A native adapter should expose panels through `nxp.ui` and the list screen through an explicit `obeliskchamber.list` interface rather than relying on a version number.

### Single binary

A Windows x64 plugin built against this header and the compatibility adapter loaded in one EmblemWeave development build. That does not prove it loads in the other two packaged runtimes. The common ABI makes it plausible; the matrix records it as unproven.

## 7. Next verification steps

1. Build `trinity.dll` after migration and run the host harness.
2. Load `trinity.dll` in the EmblemWeave development runtime and capture the panel.
3. Build the Tauri launcher page in an upstream checkout and inspect the rendered route.
4. Obtain a fefw-recomp source build at API 4, then load the same DLL.
5. Obtain an ObeliskChamber source build and record the v5 table layout before calling it.
6. Add a native EmblemWeave adapter that calls `nxp_get_plugin` directly.
7. Build on Linux and test `dlopen` in one host before claiming Linux support.

## 8. Reporting rule

A future update must append evidence, not replace a lower level with a higher one without a test. "Source integrated" is not "runtime verified". "Panel rendered" is not "gameplay works". The matrix is the public claim.
