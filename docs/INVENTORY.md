# Development inventory and migration record

Date: 25 November 2026

This is the audit that established Trinity as the central development home. It records what was found, what was migrated, what was deliberately left in place, and what remains only a design.

## 1. Workspaces inspected

| Workspace | What it contains | State |
| --- | --- | --- |
| `unified-plugin-sdk/` in the development workspace | Phase 0 NXP header, compatibility adapter, demo plugin, harness, build scripts, assessment, proposal and runtime evidence | Not a Git repository. All useful files migrated here. |
| `EmblemWeave/` | The GPL-3.0-or-later EmblemWeave checkout, its egui launcher and the fefw-recomp runtime subtree | Branch `main`, commit `c302f0abc73c4d1afa13f75ae1462bbe4cacb058`. One untracked build directory, preserved. Other worktrees and a stash exist and were not touched. |
| `X:\fefw-recomp-0.2.13` | Packaged Windows release, no source or Git metadata | Inspected only. No files changed. |
| `X:\ObeliskChamber-0.3.2` | Packaged Windows release, no source or Git metadata | Inspected only. No files changed. |
| `.tmp/upstream-fefw/` | Shallow clone of metrist38/fefw-recomp, current `master` | Used to inspect the Tauri launcher source. Not the packaged 0.2.13 source. |
| `.tmp/upstream-oc/` | Shallow clone of xinthefreefallxai/ObeliskChamber, current `master` | Used to inspect the Tauri launcher and mod API source. Not the packaged 0.3.2 source. |
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
| Trinity brand work | `assets/brand/` | Vector mark and lockup derived from the supplied logo. |

The migrated NXP files keep their `GPL-3.0-or-later` SPDX identifiers. The top-level repository license remains the MIT license established by the repository owner. See `THIRD_PARTY.md`.

## 3. Material that was deliberately not copied

| Material | Reason |
| --- | --- |
| The full EmblemWeave, fefw-recomp or ObeliskChamber repositories | Trinity is a shared framework, not a fork or replacement. Copying a host would duplicate its history, assets and unrelated changes. |
| Host game dumps, saves, logs, build products and binaries | Not source, potentially large or private, and not needed by the framework. |
| The packaged launchers from `X:\fefw-recomp-0.2.13` and `X:\ObeliskChamber-0.3.2` | Binary artifacts, not source. Their exact API layouts are not established. |
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

The upstream launcher clones are newer than the packaged releases. They prove that each launcher is a Tauri 2 + Vite application with an existing `PAGES` table, not that the packaged 0.2.13 or 0.3.2 binary has the same source.

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
| Launcher catalog surface | Implemented for fefw-recomp and ObeliskChamber | Vite builds still need to be run in a host checkout |
| EmblemWeave native page | Design only | egui integration belongs in the EmblemWeave tree |
| Native NXP host adapter | Design only | Requires host source changes and maintainer review |
| Gameplay interface | Not designed beyond a boundary note | No gameplay claim |
| Linux binary | None | Not built |

## 6. First implementation milestone

The first milestone is a repository that can produce a real plugin, validate its package, exercise it through the host harness, and load the same DLL in the one available runtime. The next milestone is a native EmblemWeave adapter that calls `nxp_get_plugin` without `nxmod_init`.

The launcher UI is deliberately not on the critical path for the ABI. It consumes the same catalog, but a launcher cannot prove runtime compatibility.

## 7. Preservation record

- No host source file was modified.
- No host branch, worktree or stash was changed.
- The untracked `engine/fefw-recomp/runtime/build_full/` directory was left in place.
- The source assessment and proposal were retained verbatim apart from a migration banner and path corrections.
- Runtime evidence images and the log were copied byte for byte.
- The supplied logo was not committed as a raster because its original resolution and provenance are not established; the vector mark preserves the identity.
