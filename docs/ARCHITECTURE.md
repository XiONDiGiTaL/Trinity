# Trinity architecture

## 1. Purpose

Trinity is a shared plugin framework for three independent recompilation projects. It gives plugin authors one source-level contract without forcing the hosts to share a UI toolkit, a launcher technology or a game runtime.

The architecture has four rules:

1. The host keeps ownership of its process, UI, game state and update path.
2. A plugin declares what it needs and degrades when a host lacks a capability.
3. Existing `NxModApi` mods keep working during the transition.
4. Every compatibility claim names the level that was actually verified.

## 2. Layers

```text
Plugin package
  plugin.json             metadata, ABI range, host list, entry point
  Trinity.dll / libTrinity.so   code
  assets/                 optional plugin-owned data
        |
        | native module load
        v
Host runtime
  native NXP adapter      or   NXMod compatibility adapter
        |
        | NxpHostApi + optional interface tables
        v
Plugin
  init / shutdown
  nxp.ui / nxp.settings / nxp.files / nxp.frame
        |
        | separate process, files only
        v
Launcher
  scan / validate / install / enable / disable / explain
```

The launcher never loads plugin code. The runtime never trusts the launcher's manifest as an ABI proof. The manifest describes a package; the loaded descriptor proves what the module exports.

## 3. Public ABI

The only public header is `include/nxp/nxp.h`.

A plugin exports:

```c
const NxpPluginDesc* nxp_get_plugin(uint32_t host_abi_version);
```

The descriptor carries:

- NXP ABI version and descriptor size;
- stable plugin id, name and version;
- a short description;
- `init` and `shutdown`.

The host validates the ABI version, the descriptor size and the init pointer before calling `init`. A descriptor that fails validation is not partially trusted.

The host root table carries:

- ABI version and size;
- `host_id`, `host_name`, `host_version`;
- `plugin_dir`;
- capability bits;
- a logging function;
- `get_interface`.

Optional interfaces are discovered with:

```c
const void* nxp_get_interface(const NxpHostApi* host, const char* name, uint32_t version);
```

A host returns the requested version or a compatible later version, or `NULL`. A missing interface is not an initialization failure.

## 4. Versioning rules

- Every cross-boundary table starts with `size` and `version`.
- Members are appended only. Reordering or removing a member is a breaking change.
- A caller that reads a member added after its target version must check `size` first.
- A breaking UI change gets a new interface name or major version, not a silent v1 change.
- `NXP_ABI_VERSION` changes only for a breaking change to the root contract.
- `NXMOD_API_VERSION` is not changed merely to advertise NXP.
- Existing `nxmod_init` mods remain loadable.

The current interfaces are all version 1:

| Interface | Functions |
| --- | --- |
| `nxp.ui` | `panel_add`, `panel_visible` |
| `nxp.settings` | integer and string get/set, flush |
| `nxp.files` | scoped read and write |
| `nxp.frame` | one per-presented-frame callback |

## 5. Lifecycle

```text
loader finds module
  -> resolve nxp_get_plugin
  -> validate descriptor
  -> build host table
  -> plugin init(host)
       -> discover interfaces
       -> load settings
       -> register panel / frame callback
  -> host runs
       -> panel callbacks on the presenting thread while visible
       -> frame callbacks on the game main thread
  -> host calls shutdown once on orderly teardown
  -> process exit is also tolerated
```

The current host path does not unload modules. An update applies on restart. The compatibility adapter uses process-lifetime allocations for panel and frame bindings; that is bounded only because there is no reload.

A plugin that returns an error from `init` is not considered loaded. The host logs the result and must not call `shutdown` for a failed init.

## 6. Threading and memory

| Entry point | Thread |
| --- | --- |
| `nxp_get_plugin` | Any thread; called once during load |
| `init` / `shutdown` | Loader or game main thread |
| `nxp.ui` panel callback | Presenting thread while the panel is visible |
| `nxp.frame` callback | Game main thread once per presented frame |
| `nxp.settings` | Any thread; the host serializes access |
| `nxp.files` | Any thread; host-defined synchronization |

Rules:

- No C++ object, exception, RTTI type, `std::string`, allocator pointer or `FILE*` crosses the C boundary.
- The plugin owns memory it allocates and frees it with its own allocator.
- The host copies any plugin string it keeps.
- The plugin does not retain a host temporary after the call that supplied it.
- The host owns the UI canvas for the duration of a panel callback.
- A panel callback must not block, call guest functions or mutate game state. Queue that work for `nxp.frame`.
- A plugin must not throw across the boundary. A native host should still guard callbacks where the platform allows it.

## 7. UI model

NXP v1 uses an immediate-mode canvas. The plugin calls widgets in order; the host draws them with its own fonts, spacing, colours, focus order and controller navigation. This maps directly onto the existing `NxModUi` v4 path and is the smallest honest bridge.

The host owns:

- panel placement and visibility;
- type, palette, spacing and scaling;
- keyboard, mouse and controller navigation;
- clipping and safe areas;
- notification style;
- accessibility behaviour.

The plugin owns:

- content and ordering;
- stable widget IDs using the host's documented suffix convention;
- values and validation;
- localization of its own strings.

NXP v1 is not the final UI. It has no tabs, tables, text inputs, dropdowns, dialogs or custom rendering. `docs/NXP.md` records the proposed v2 composite model. It is not implemented.

## 8. Settings and files

The compatibility adapter implements settings over a per-plugin `.nxp-settings.ini` in the plugin directory. Keys are ASCII identifiers. The settings map is mutex-protected. Each set currently flushes immediately; a future version should debounce slider-backed writes.

The file interface is scoped to `plugin_dir` by the adapter. It rejects absolute paths, drive paths, `.` components and `..` components. This prevents accidental path escapes through the interface, but it is not a security boundary: a native DLL can call the platform file APIs directly.

A future native adapter should provide a stable per-plugin data directory rather than storing user data next to executable code.

## 9. Launcher and runtime separation

The launcher:

- reads `plugin.json`;
- validates identity, version, ABI range, host list, entry path and dependencies;
- displays compatibility and errors;
- installs with traversal protection and hash verification;
- enables or disables by folder convention;
- preserves settings across updates;
- never loads a plugin DLL.

The runtime:

- loads the module with platform-specific flags;
- validates the NXP descriptor;
- builds the real host table;
- enforces thread and path rules;
- reports structured errors where available.

The `_` and `-` folder prefixes already disable a mod in the shared runtime lineage. The launcher should use that convention rather than inventing a second enabled flag.

## 10. Prefixes

Canonical names use a dotted, lowercase namespace. The first segment is the owner.

- `trinity.` is portable shared functionality.
- `nxp.` is the compatibility ABI.
- `emblemweave.`, `fefw-recomp.` and `obeliskchamber.` are host-owned.
- Short aliases are accepted as input but not written as canonical names.
- Reserved names such as `core`, `ui`, `settings`, `files` and `frame` cannot be claimed as a first segment by a plugin.

This follows the lookup shape proposed in fefw-recomp issue #165 while keeping NXP's host lookup separate from `NxModApi.get_interface`. See `PREFIXES.md`.

## 11. Cross-platform

The ABI is C and the public header uses only fixed-width integers, size types and function pointers. That removes C++ name mangling and object layout, but not every binary risk.

Windows:

- `LoadLibraryExW` with a restricted search path;
- `GetProcAddress` for `nxp_get_plugin`;
- UTF-8 at the boundary and UTF-16 for Win32 calls;
- one binary per architecture.

Linux:

- `dlopen` with `RTLD_NOW | RTLD_LOCAL`;
- `dlsym`;
- `-fvisibility=hidden` plus explicit default visibility for the entry point;
- one `.so` per architecture;
- path and case behaviour tested on the target runtime.

Unresolved risks:

- structure packing and alignment;
- floating-point ABI differences;
- exception handling across the boundary;
- sanitizer or control-flow instrumentation;
- CRT and allocator differences;
- ARM64 support;
- host and plugin built against different copies of the header.

No Linux binary has been built. No single-binary-for-everything claim is made.

## 12. Security model

A native plugin runs with the host's full rights. The file sandbox and manifest checks reduce accidents; they do not contain malicious code. The ecosystem must treat native plugins as trusted software.

The launcher should still:

- reject absolute paths, `..`, symlinks and zip-slip entries;
- verify package hashes;
- stage an install before replacing a working version;
- preserve user settings on upgrade;
- show the exact incompatibility reason;
- link to runtime diagnostics.

Code signing, a reviewed registry and automatic revocation are future work.

## 13. Component map

| Component | Location | Status |
| --- | --- | --- |
| Public ABI | `include/nxp/nxp.h` | v1 implemented |
| Compatibility host | `hosts/compat/nxmod_adapter.cpp` | Implemented and runtime-tested in EmblemWeave |
| Reference plugin | `plugins/trinity_core/` | Implemented; build and reload after migration |
| Minimal example | `examples/hello_panel/` | Implemented |
| Host harness | `tests/nxp_host_test.cpp` | Implemented |
| Manifest validator | `tools/validate_plugin.py` | Implemented |
| Prefix core | `shared/prefix-core.mjs` | Implemented |
| Catalog | `shared/catalog.json` | Source of truth for launcher listings |
| Launcher surface | `ui/` | Shared content and styling hooks |
| Native EmblemWeave page | Host checkout | Not implemented here |
| Native NXP adapters | Host checkouts | Not implemented |
| Gameplay interface | None | Not designed |

## 14. Decisions still open

1. Whether a native host exposes NXP through `NxpHostApi` directly or through `NxModApi.get_interface` underneath.
2. The final neutral name for ObeliskChamber's list-screen interface.
3. Whether `plugin.json` is accepted as the common launcher manifest alongside host-specific metadata.
4. The support window for `nxmod_init` after native NXP adoption.
5. The ownership and shape of a future gameplay interface.
6. The first Linux target and toolchain.
7. Whether package signing is required before a public registry exists.
