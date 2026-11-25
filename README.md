# Trinity

**The Three Houses Of Innovation**

**Three projects. Three launchers. One shared plugin ecosystem.**

Trinity is the shared plugin framework for three independent *Fire Emblem: Fortune's Weave* projects:

- [EmblemWeave](https://github.com/XiONDiGiTaL/EmblemWeave)
- [fefw-recomp](https://github.com/metrist38/fefw-recomp)
- [ObeliskChamber](https://github.com/xinthefreefallxai/ObeliskChamber)

The host applications remain independent. Trinity defines the common contract between them: a versioned C ABI, a compatibility adapter, host-specific integration layers, a shared catalog and a prefix registry for interface names.

This repository is the central development home for that framework. It is not a replacement for any host project.

## Status

| Area | State |
| --- | --- |
| Public NXP ABI | Implemented, version 1 |
| Compatibility adapter for `NxModApi` | Implemented; loaded and exercised in an EmblemWeave development build |
| Reference plugin | Implemented, built and loaded by the EmblemWeave development runtime |
| Host adapter design | Documented for all three hosts |
| Launcher catalog surface | fefw-recomp and ObeliskChamber adapters build and render in local upstream clones; native EmblemWeave design documented |
| fefw-recomp runtime validation | Not run locally; the installed package is API 3 and has no prepared game in this workspace |
| ObeliskChamber runtime validation | Not run locally; the installed package is API 5 and has no prepared game in this workspace |
| Linux build | Not built or loaded |

Compilation is not loading. Loading is not rendering. Rendering is not gameplay. The support matrix in [docs/COMPATIBILITY.md](docs/COMPATIBILITY.md) records each level separately.

## What is in the repository

| Path | Purpose |
| --- | --- |
| `include/nxp/nxp.h` | The public C ABI. Plugins compile against this header only. |
| `hosts/compat/` | The `NxModApi` compatibility host linked into a plugin DLL. It exports `nxmod_init` for hosts that do not load NXP natively. |
| `plugins/trinity_core/` | The reference Trinity plugin: host discovery, settings, a native panel, a frame hook and the prefix registry. |
| `examples/hello_panel/` | The smallest working NXP plugin. |
| `tests/` | Standalone host harness, manifest tests and prefix tests. |
| `tools/` | Windows build script, launcher sync and manifest validation. |
| `shared/` | The canonical catalog and prefix registry shared by launchers and plugins. |
| `ui/` | The shared launcher surface. It supplies content, not a host visual language. |
| `hosts/launchers/` | Host-specific launcher adapters. |
| `docs/` | Architecture, compatibility, migration inventory, proposals and runtime evidence. |
| `assets/brand/` | Trinity mark and lockup. |

## Design in one page

A plugin exports one function:

```c
const NxpPluginDesc* nxp_get_plugin(uint32_t host_abi_version);
```

The host validates the descriptor, calls `init`, and exposes optional interfaces through `NxpHostApi.get_interface`. The v1 interfaces are:

- `nxp.ui`
- `nxp.settings`
- `nxp.files`
- `nxp.frame`

Every interface table starts with `size` and `version`. Tables grow only at the end. A missing interface is a normal state; the plugin must degrade.

The current compatibility adapter maps NXP onto the existing `NxModApi` table, so no host API version needs to change for the first working plugins. A native host adapter can expose the same `NxpHostApi` directly later. The legacy `nxmod_init` path stays supported throughout the transition.

The launcher and the runtime remain separate processes. Launchers read manifests and manage files. Runtimes validate and load code. A manifest is not a security boundary.

## Prefix registry

Trinity implements the project-prefix idea from [fefw-recomp issue #165](https://github.com/metrist38/fefw-recomp/issues/165) at the shared layer:

| Owner | Canonical prefix | Purpose |
| --- | --- | --- |
| Trinity | `trinity.` | Portable services owned by the shared framework |
| NXP | `nxp.` | The compatibility ABI |
| EmblemWeave | `emblemweave.` | Host-only interfaces and game integrations |
| fefw-recomp | `fefw-recomp.` | Host-only interfaces and game integrations |
| ObeliskChamber | `obeliskchamber.` | Host-only interfaces and game integrations |

Short aliases such as `ew.`, `fefw.` and `oc.` are accepted for input and diagnostics. Canonical names are written to disk. The loader rejects unknown and doubled prefixes. Existing `NxModApi` symbols remain valid; prefixes are added at the interface registry rather than forced onto old entry points.

The machine-readable registry is [shared/prefixes.json](shared/prefixes.json). The portable classifier and its tests are [shared/prefix-core.mjs](shared/prefix-core.mjs) and [shared/prefix-core.test.mjs](shared/prefix-core.test.mjs).

## Build on Windows

Requirements:

- CMake 3.20 or newer
- Ninja
- Visual Studio 2022 or newer
- The clang-cl and lld-link binaries shipped under `EmblemWeave/tools/bin` in the development workspace

From the workspace root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Trinity/tools/build-windows.ps1
```

Outputs:

- `Trinity/build/trinity.dll`
- `Trinity/build/nxp_demo.dll`
- `Trinity/build/nxp_host_test.exe`

The compatibility adapter compiles against the public `nxrt/mod_api.h` and `nxrecomp/ctx.h` headers from an EmblemWeave checkout. Pass `-DNXP_NXMOD_ROOT=<checkout>/engine/fefw-recomp` if the checkout is elsewhere.

A native host implementation needs only `include/nxp/nxp.h`.

## Test

```powershell
ctest --test-dir Trinity/build --output-on-failure
node --test Trinity/shared/prefix-core.test.mjs
python Trinity/tools/validate_plugin.py Trinity/plugins/trinity_core/plugin.json
python -m unittest discover -s Trinity/tests -p "test_*.py"
```

The C++ harness loads the plugin DLL, supplies an NXP host with all four interfaces, checks discovery and panel registration, exercises settings and reset, runs the frame callback, calls shutdown and checks the final flush.

The EmblemWeave runtime path is documented in [docs/COMPATIBILITY.md](docs/COMPATIBILITY.md). The installed executable on this machine predates code-mod support; the current development build is the test target.

## Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Compatibility matrix](docs/COMPATIBILITY.md)
- [Development inventory and migration record](docs/INVENTORY.md)
- [NXP ABI guide](docs/NXP.md)
- [Prefix specification](docs/PREFIXES.md)
- [Technical assessment](docs/ASSESSMENT.md)
- [Maintainer proposal](docs/PROPOSAL.md)

## Licensing

The repository is MIT licensed as established by its owner. The NXP header, the compatibility adapter and the reference plugin carry `GPL-3.0-or-later`, matching the host projects from which their ABI is derived. Third-party and host-derived material is listed in [docs/THIRD_PARTY.md](docs/THIRD_PARTY.md). Do not remove an SPDX identifier from a migrated file.

## Contributing

The three host projects remain independent. Changes to a host repository require that host's maintainers and its own review process. Trinity changes belong here.

Do not claim support for a host or a platform without a matching entry in the compatibility matrix and evidence for the exact level claimed.

Independent projects, shared innovation, weaved together.
