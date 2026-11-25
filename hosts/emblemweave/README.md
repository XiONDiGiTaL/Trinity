# EmblemWeave host adapter

## Current state

The compatibility adapter loads in the EmblemWeave development runtime (`nxmod-api-4`). The Trinity Core panel registers under the host's Mods surface. This is the only host with runtime evidence.

The installed `%APPDATA%\EmblemWeave\play\fefw.exe` from 15 November 2026 is older and lacks the code-mod loader. A release build must include the newer runtime before players can use plugins.

## Native adapter plan

The native adapter belongs in the EmblemWeave runtime, not in this repository. It should:

1. Load an NXP plugin by resolving `nxp_get_plugin`.
2. Fall back to `nxmod_init` for legacy mods.
3. Build `NxpHostApi` with:
   - `host_id = "emblemweave"`;
   - the real host name and version;
   - the plugin directory;
   - the true capability mask.
4. Implement `nxp.ui` over the existing panel registry and ImGui canvas.
5. Implement `nxp.settings` with a stable per-plugin data directory.
6. Implement `nxp.files` scoped to that directory.
7. Implement `nxp.frame` over the existing frame hook.
8. Keep `NXMOD_API_VERSION` at 4 until a separate, agreed change.
9. Add loader tests for NXP-only, legacy-only and dual-entry plugins.
10. Record the exact runtime version and test level in `docs/COMPATIBILITY.md`.

## Launcher plan

The egui launcher needs a native `Page::Trinity` and a navigation group. The existing page tests require every page to belong to exactly one group. See `hosts/launchers/emblemweave/README.md`.

The launcher must not load plugin DLLs. It reads the catalog and manifest only.

## Evidence

`docs/evidence/` contains the F1 menu, the demo panel and the runtime log. The Trinity Core headless load adds the plugin identity, panel registration and prefix check lines. A windowed capture of the Trinity panel is still useful and should be added when the UI navigation is automated.
