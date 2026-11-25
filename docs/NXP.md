# NXP ABI guide

NXP is the versioned C ABI at the center of Trinity. It lets one plugin source target EmblemWeave, fefw-recomp and ObeliskChamber without making the three hosts share a UI toolkit or a launcher.

NXP v1 is deliberately small. It covers host discovery, logging, a native panel, settings, plugin-scoped files and a frame callback. It does not expose game memory or gameplay hooks. Those remain host capabilities and a future optional interface.

## 1. Entry point

A plugin exports one function:

```c
const NxpPluginDesc* nxp_get_plugin(uint32_t host_abi_version);
```

The host calls it once during load, then checks:

- `desc != NULL`;
- `desc->abi_version == NXP_ABI_VERSION`;
- `desc->size >= NXP_PLUGIN_DESC_V1_SIZE`;
- `desc->init != NULL`.

Only then does the host call `desc->init(host)`.

A host that supports orderly shutdown calls `desc->shutdown()` once. A plugin must also tolerate process exit without shutdown. The current loader never unmaps a plugin DLL.

## 2. Descriptor

```c
typedef struct NxpPluginDesc {
    uint32_t abi_version;
    uint32_t size;
    const char* id;
    const char* name;
    const char* version;
    const char* description;
    int (*init)(const NxpHostApi* host);
    void (*shutdown)(void);
} NxpPluginDesc;
```

The strings are static and remain valid for the module's lifetime. `id` is a stable identifier such as `org.xion.trinity.core`, not a display name.

## 3. Host table

```c
typedef struct NxpHostApi {
    uint32_t abi_version;
    uint32_t size;
    const char* host_id;
    const char* host_name;
    const char* host_version;
    const char* plugin_dir;
    uint32_t capabilities;
    NxpLogFn log;
    NxpGetInterfaceFn get_interface;
} NxpHostApi;
```

`host_id` values currently observed:

- `emblemweave`
- `fefw-recomp`
- `obelisk-chamber`

A compatible host may use another id. Plugins must use capabilities for behaviour, not the host name.

`plugin_dir` is the absolute directory the host loaded the module from. It is UTF-8. A plugin should use `nxp.files` for relative asset access rather than concatenate paths.

`get_interface` is authoritative. Capability bits are a cheap first look.

## 4. Interface lookup

```c
const void* nxp_get_interface(const NxpHostApi* host,
                              const char* name,
                              uint32_t version);
```

A host returns a table whose first two members are `size` and `version`, or `NULL`. Names are ASCII and case-sensitive. A request for a version the host does not implement returns `NULL`; the plugin must degrade.

Version 1 interfaces:

| Name | Table | Functions |
| --- | --- | --- |
| `nxp.ui` | `NxpUiApi` | `panel_add`, `panel_visible` |
| `nxp.settings` | `NxpSettingsApi` | `get_int`, `set_int`, `get_string`, `set_string`, `flush` |
| `nxp.files` | `NxpFileApi` | `read_file`, `write_file` |
| `nxp.frame` | `NxpFrameApi` | `on_frame` |

The UI canvas is `NxpUiCanvas`. It has 14 functions:

```text
text, text_disabled, text_colored, checkbox, slider_float, slider_int,
button, same_line, separator, begin_disabled, end_disabled, tree_node,
tree_pop, tooltip
```

The host owns the canvas for the duration of the callback. The plugin must close any `begin_disabled` or `tree_node` it opens. The EmblemWeave path already does this on the host side.

## 5. Capabilities

| Bit | Meaning |
| --- | --- |
| `NXP_CAP_HOST_INFO` | Host identity is present |
| `NXP_CAP_LOG` | `log` is present |
| `NXP_CAP_UI_PANEL` | A panel can be added |
| `NXP_CAP_UI_WIDGETS` | The v1 canvas widgets are available |
| `NXP_CAP_SETTINGS` | Settings are available |
| `NXP_CAP_FILES` | Plugin-scoped files are available |
| `NXP_CAP_FRAME` | A frame callback is available |

A bit may be broader than a particular table version. Always check the table size and the function pointer you need.

## 6. Thread contract

| Entry point | Thread |
| --- | --- |
| `nxp_get_plugin` | Any thread, called once during load |
| `init` / `shutdown` | Loader or game main thread |
| `nxp.ui` panel callback | Presenting thread while visible |
| `nxp.frame` callback | Game main thread once per presented frame |
| `nxp.settings` | Any thread; the adapter serializes access |
| `nxp.files` | Any thread; host-defined synchronization |

A panel callback must not block or mutate game state. Queue that work for `nxp.frame`. The panel thread may read and write plugin-local state.

## 7. Memory and ownership

- No C++ object, exception, RTTI type, `std::string`, allocator pointer or `FILE*` crosses the boundary.
- No allocation crosses the boundary unless an interface says so.
- The plugin frees memory it allocates with its own allocator.
- The host copies any plugin string it keeps.
- The plugin does not retain a temporary host string after the call.
- The host owns the UI canvas and host table for the documented lifetime.
- The compatibility adapter keeps the host table in static storage for the process lifetime. This fixes a real bug where a stack-local table left the panel with garbage host strings.

## 8. Settings

The compatibility adapter stores settings in `.nxp-settings.ini` inside the plugin directory. Keys are ASCII identifiers `[A-Za-z0-9_.-]`. Values are UTF-8 without embedded newlines.

The adapter serializes the map with a mutex. It flushes on every set in v1. A slider drag therefore causes more disk writes than necessary; debouncing is a known improvement, not a compatibility change.

A native adapter should use a stable per-plugin data directory rather than code-adjacent storage.

## 9. Files

`read_file` returns the whole file size, even when the buffer is smaller, or `-1`. A caller can pass `NULL` with capacity `0` to query the size.

`write_file` replaces the file and returns `1` on success.

The adapter resolves paths relative to `plugin_dir` and rejects:

- absolute paths;
- drive paths;
- `.` components;
- `..` components;
- empty path components.

This is accident prevention, not a security sandbox. A native DLL can call the platform file APIs directly.

## 10. Compatibility adapter

The adapter lives at `hosts/compat/nxmod_adapter.cpp`. It is linked into a plugin DLL and exports `nxmod_init`, which the existing runtimes already call.

It:

1. detects the host id from the module path;
2. builds `NxpHostApi` from `NxModApi`;
3. exposes only the interfaces the host actually has;
4. exports `nxp_get_plugin` from the plugin source;
5. keeps a static host table for the process lifetime.

On `NxModApi` v3 it exposes settings, files and frame, but no UI. On v4 and v5 it exposes `nxp.ui` through `add_panel` and `panel_visible`.

The adapter does not unload. It installs an `atexit` shutdown hook.

`tests/compat_adapter_test.cpp` links the adapter and a probe plugin into one
executable and runs it with simulated API 3, 4 and 5 tables. It checks that v3
exposes settings, files and frame but no UI; that v4 and v5 expose the panel;
that settings and files round-trip through the host callbacks; and that panel
and frame callbacks cross the trampolines. These are mapping tests, not a game
runtime test.

The path-derived host id is a known limitation. A native adapter must pass the true identity. A future adapter could accept an injected host id or probe a host-provided interface.

## 11. Reference plugin

`plugins/trinity_core/trinity_plugin.cpp` implements:

- descriptor validation and host discovery;
- a native Trinity panel;
- two persisted settings;
- a frame heartbeat;
- the prefix registry check.

The panel is registered as **Trinity** under the host's Mods surface. On EmblemWeave it appears in F1 > Advanced > Mods.

## 12. Build

From the workspace root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Trinity/tools/build-windows.ps1
```

The build expects:

```text
../EmblemWeave/engine/fefw-recomp/runtime/include/nxrt/mod_api.h
../EmblemWeave/engine/fefw-recomp/recomp/include/nxrecomp/ctx.h
```

Pass `-DNXP_NXMOD_ROOT=<checkout>/engine/fefw-recomp` for a different layout.

Outputs:

```text
Trinity/build/Trinity.dll
Trinity/build/NxpDemo.dll
Trinity/build/nxp_host_test.exe
```

On Linux the equivalent outputs are `build-linux/libTrinity.so`, `build-linux/libNxpDemo.so` and `build-linux/nxp_host_test`; the same five CTest cases pass.

## 13. Installing for an EmblemWeave runtime test

```text
%APPDATA%\EmblemWeave\play\mods\TrinityCore\exefs\Trinity.dll
%APPDATA%\EmblemWeave\play\mods\TrinityCore\plugin.json
```

The runtime discovers mod folders under `[fs] mods`. A folder beginning with `_` or `-` is disabled. EmblemWeave builds that contain the bundle install this folder on first start; the manual path is for older launchers and development builds.

The installed EmblemWeave play executable from 15 November 2026 lacks `nxmod_init`. Use a development build that contains the code-mod loader.

## 14. What is not in v1

- Gameplay hooks, guest memory, `call_guest`, file edits;
- composite UI such as tabs, tables, selects, text inputs and dialogs;
- structured diagnostics;
- dependency resolution or load ordering;
- in-process reload or unload;
- exception containment by the host;
- a Linux GUI game host (the Linux `.so` is loaded by the native harness, not a game);
- package signing or a registry;
- custom rendering.

These are roadmap items. A plugin that needs them is host-specific until an optional interface exists.

## 15. Rules for a native host adapter

1. Include `include/nxp/nxp.h` only.
2. Load the platform module and resolve `nxp_get_plugin`.
3. Validate the descriptor before calling `init`.
4. Build a host table with the true id, name, version and plugin directory.
5. Return only interfaces the host actually implements.
6. Check table sizes before reading newer members.
7. Drive panel and frame callbacks on the documented threads.
8. Close any UI group a plugin left open.
9. Contain exceptions at the boundary.
10. Keep `nxmod_init` working for legacy mods.
11. Do not bump a host API version merely to advertise NXP.
12. Record the exact host version and test result in `COMPATIBILITY.md`.
