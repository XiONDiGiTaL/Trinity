# Development inventory and migration record

Date: 25 November 2026

This is the audit that established Trinity as the central development home. It records what was found, what was migrated, what was deliberately left in place, and what remains only a design.

## 1. Workspaces inspected

| Workspace | What it contains | State |
| --- | --- | --- |
| `unified-plugin-sdk/` in the development workspace | Phase 0 NXP header, compatibility adapter, demo plugin, harness, build scripts, assessment, proposal and runtime evidence | Not a Git repository. All useful files migrated here. |
| `EmblemWeave/` | The GPL-3.0-or-later EmblemWeave checkout, its egui launcher and the fefw-recomp runtime subtree | Branch `main`, commit `c302f0abc73c4d1afa13f75ae1462bbe4cacb058`. Native Trinity page added in the host tree; launcher rebuilt for the capture. One untracked build directory, preserved. Other worktrees and a stash exist and were not touched. |
| `X:\fefw-recomp-0.2.13` | Packaged Windows release, no source or Git metadata | Launcher replaced with a 0.2.13 build carrying Trinity; game runtime and data untouched. |
| `X:\ObeliskChamber-0.3.2` | Packaged Windows release, no source or Git metadata | Launcher replaced with a 0.3.2 build carrying Trinity; game runtime and data untouched. |
| `.tmp/upstream-fefw/` | Shallow clone of metrist38/fefw-recomp, current `master` | Used to inspect and integrate the Tauri launcher. A separate `v0.2.13` worktree was built with Trinity for the actual window. |
| `.tmp/upstream-oc/` | Shallow clone of xinthefreefallxai/ObeliskChamber, current `master` | Used to inspect, integrate and build the actual 0.3.2 Tauri window. |
| `X:\FCC Desktop` | Electron application used as visual reference | Design tokens and model-library layout inspected. No code copied. |

The host repositories keep their own branches, worktrees, stashes and recovery folders. No reset, clean, checkout or history rewrite was performed.

## 2. Material that was migrated

| Source | Destination | Reason |
| --- | --- | --- |
| `unified-plugin-sdk/include/nxp/nxp.h` | `include/nxp/nxp.h` | The public ABI is the foundation of the shared framework. |
| `unified-plugin-sdk/src/nxmod_adapter.cpp` | `hosts/compat/nxmod_adapter.cpp` | It is the only currently runtime-validated bridge to all three existing loaders. |
| `unified-plugin-sdk/examples/hello_panel/` | `examples/hello_panel/` | Minimal working example and the loader smoke test. |
| `unified-plugin-sdk/tests/host_test.cpp` | `tests/nxp_host_test.cpp` | Portable host harness. It now exercises both the demo and Trinity Core. |
| `unified-plugin-sdk/scripts/build-windows.ps1` | `tools/build-windows.ps1` | Reproducible Windows build path. |
| `unified-plugin-sdk/ASSESSMENT.md` | `docs/ASSESSMENT.md` | Technical assessment and verification ledger. |
| `unified-plugin-sdk/PROPOSAL.md` | `docs/PROPOSAL.md` | Maintainer-facing proposal. Not posted, not accepted. |
| `unified-plugin-sdk/docs/evidence/` | `docs/evidence/` | Real EmblemWeave F1 menu, panel capture and runtime log. |
| Launcher page prototype from the earlier Trinity work | `ui/`, `shared/`, `hosts/launchers/` | Shared catalog, prefix registry, host-specific launcher presentation. |
| Trinity brand work | `assets/brand/` | Authoritative logo supplied by the project owner. |

The migrated NXP files keep their `GPL-3.0-or-later` SPDX identifiers. The top-level repository license remains the MIT license established by the repository owner. See `THIRD_PARTY.md`.

## 3. Material that was deliberately not copied

| Material | Reason |
| --- | --- |
| The full EmblemWeave, fefw-recomp or ObeliskChamber repositories | Trinity is a shared framework, not a fork or replacement. Copying a host would duplicate its history, assets and unrelated changes. |
| Host game dumps, saves, logs, build products and binaries | Not source, potentially large or private, and not needed by the framework. |
| The packaged launchers from `X:\fefw-recomp-0.2.13` and `X:\ObeliskChamber-0.3.2` | Binary artifacts, not source. Their launcher executables were replaced for the page captures; game runtimes and data were not copied. |
| Generated files under `EmblemWeave/target/` and `_recovery/` | Build output and preserve-only working material. |
| FCC Desktop application code | Only its visual language informed the launcher surface. No code or assets were copied. |

## 4. Host API inventory

| Property | EmblemWeave | fefw-recomp | ObeliskChamber |
| --- | --- | --- | --- |
| Local source | Full checkout | Upstream clone, newer than the package | Upstream clone, newer than the package |
| Packaged version inspected | Development build from current source | 0.2.13 | 0.3.2 |
| `NXMOD_API_VERSION` | 4 | 3 in the package, 4 planned upstream | 5 |
| Entry points | `nxmod_early`, `nxmod_init` | Same | Same |
| Panel API | `add_panel`, `panel_visible` | Absent in the packaged API 3 | Present; `open_list` shares the same table slot |
| Panel rendering | ImGui F1 menu, Advanced > Mods | Not available on API 3 | ImGui menu and overlay; exact package layout not run |
| Frame callback | `on_frame` | `on_frame` | `on_frame` |
| File APIs | Raw host paths | Same | Same |
| Hook kinds | `inline`, `midhook`, `entryhook` | Same | Same |
| Unload | No; DLL stays mapped | Same | Same |
| NXP runtime evidence | Loaded, panel rendered, settings persisted | None | None |

The fefw-recomp v0.2.13 tag and the current ObeliskChamber 0.3.2 source were built with Trinity and installed as launchers. The game runtimes and their API layouts were left unchanged.

## 5. Current repository components

| Component | State | Evidence |
| --- | --- | --- |
| NXP ABI v1 | Implemented | Header compiles; harness exercises it |
| Compatibility adapter | Implemented | Loaded by the EmblemWeave development build |
| Trinity Core plugin | Implemented in this repository | Built; loaded and registered a panel in the EmblemWeave development runtime |
| Hello panel demo | Implemented | CTest harness |
| Manifest validator | Implemented | Python unit tests |
| Prefix classifier | Implemented in JavaScript | Node tests |
| Prefix classifier in C++ | Implemented inside Trinity Core | Registry check logs on init |
| Launcher catalog surface | Implemented and captured in actual fefw-recomp 0.2.13 and ObeliskChamber 0.3.2 windows | Tauri builds pass; captures are in `docs/evidence/` |
| EmblemWeave native page | Implemented in the host tree | Captured from the 0.1.43 release launcher; drift tests pass |
| Native NXP host adapter | Design only | Requires host source changes and maintainer review |
| Gameplay interface | Not designed beyond a boundary note | No gameplay claim |
| Linux binary | None | Not built |

## 6. First implementation milestone

The first milestone is a repository that can produce a real plugin, validate its package, exercise it through the host harness, and load the same DLL in the one available runtime. The next milestone is a native EmblemWeave adapter that calls `nxp_get_plugin` without `nxmod_init`.

The launcher UI is deliberately not on the critical path for the ABI. It consumes the same catalog, but a launcher cannot prove runtime compatibility.

## 7. Preservation record

- The consolidation itself copied no host source. The EmblemWeave native page is a separate host-tree change; the `.tmp` launcher checkouts carry local Trinity integrations used for the captures.
- No host branch or stash was changed. A separate worktree was created at the fefw-recomp v0.2.13 tag for the build.
- The untracked `engine/fefw-recomp/runtime/build_full/` directory was left in place.
- The source assessment and proposal were retained verbatim apart from a migration banner and path corrections.
- The historical F1 menu, mod panel and runtime logs were preserved. The three launcher page images were replaced with actual window captures.
- The supplied logo is committed as `assets/brand/trinity.png` and is the only Trinity brand artwork used by the repository.

## 8. 0.2.0 update

- The plugin is versioned 0.2.0 and builds as `Trinity.dll` on Windows x64 and
  `libTrinity.so` on Linux x64.
- The Windows build loaded alongside the original Quick Save 1.0.0 and Turn
  Control 0.3.0 DLLs in the EmblemWeave development runtime. This is load/init
  evidence; no save, turn, facility or other gameplay action was exercised.
- The Linux build is a native ELF64 shared object, checked with `readelf` and
  `nm -D` and loaded by the native `nxp_host_test` harness. There is still no
  Linux game launcher in this workspace, so Linux gameplay remains unverified.
- EmblemWeave embeds the Windows payload and installs or updates only
  `TrinityCore` on first start. Three Rust tests cover install, version guard
  and settings preservation.
- The official-release helpers and the archive checksums are in `tools/` and
  `docs/INSTALL.md`.
