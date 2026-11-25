# NXMod compatibility host

`nxmod_adapter.cpp` links into an NXP plugin DLL and exports `nxmod_init`. It lets the existing EmblemWeave, fefw-recomp and ObeliskChamber loaders run an NXP plugin without changing their API version.

It is the phase 0 bridge. It is not the final native host adapter.

## What it exposes

| Host capability | NXP interface |
| --- | --- |
| `log` | `NxpHostApi.log` |
| `add_panel`, `panel_visible` on API 4 or 5 | `nxp.ui` |
| `read_file`, `write_file` on API 3 or later | `nxp.settings` and `nxp.files` |
| `on_frame` | `nxp.frame` |

On API 3 the plugin runs headless. It can still persist settings, read and write plugin-scoped files and use the frame hook. It must not pretend a panel exists.

## Host identity

The adapter derives `host_id` from the loaded module path. It recognises `emblemweave`, `fefw-recomp` and `obeliskchamber`, and falls back to `nxmod-compatible`.

A renamed executable can therefore be misidentified. A native adapter must pass the true identity. Plugins must use capabilities, not the path-derived name, for behaviour.

## Lifetime

The adapter keeps:

- `NxpHostApi` in static storage;
- plugin and host strings for the process lifetime;
- panel and frame bindings allocated with `new`.

The runtime never unloads a plugin DLL, so the bindings are bounded by process lifetime. This is a leak if reload is ever supported. Do not add reload without changing the lifetime model.

## Settings

Settings are stored in `.nxp-settings.ini` beside the plugin. Every `set_int` or `set_string` flushes immediately. The map is mutex-protected. See `docs/NXP.md` for the format.

## File scope

The adapter rejects absolute paths, drive paths, empty components, `.` and `..` before calling the host file API. It is accident prevention, not a security boundary.

## Build

The adapter is compiled into `trinity_core` and `nxp_demo` by the root `CMakeLists.txt`. It needs the public host headers, not the host source:

```text
../EmblemWeave/engine/fefw-recomp/runtime/include/nxrt/mod_api.h
../EmblemWeave/engine/fefw-recomp/recomp/include/nxrecomp/ctx.h
```

## Limits

- Windows only in the current implementation (`LoadLibrary`/Win32 file calls live in the host, but the adapter itself uses Win32 for path detection).
- No native export path for `nxp_get_plugin`; that comes from the plugin source.
- No structured diagnostics.
- No exception guard.
- No unload.
- No Linux build.
- `add_panel` on ObeliskChamber API 5 is source-consistent but not runtime-verified in the packaged host.
