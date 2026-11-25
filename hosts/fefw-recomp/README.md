# fefw-recomp host adapter

## Current state

- Packaged release inspected: `X:\fefw-recomp-0.2.13`.
- `NXMOD_API_VERSION` 3 in that package.
- `nxmod_init` and `nxmod_early` are present.
- `add_panel` is absent, so the packaged host cannot show an NXP panel.
- `on_frame`, file APIs, hook sites and `call_guest` follow the shared lineage.
- Upstream `master` is newer than the package and is the basis for the launcher source inspection. It is not proof that the packaged 0.2.13 binary contains the same code.
- No prepared game was available, so the Trinity DLL was not loaded.

## Adapter plan

On API 3:

- expose `nxp.settings`, `nxp.files`, `nxp.frame` and logging;
- do not expose `nxp.ui`;
- keep the legacy `nxmod_init` path;
- run the plugin headless.

On API 4, when the host provides `add_panel` and `panel_visible`:

- expose `nxp.ui`;
- pass the real host id and version;
- add the same host harness test that EmblemWeave uses.

## Launcher

`hosts/launchers/fefw/trinity.js` is a page adapter for the upstream Tauri launcher. It expects `ui/trinity-ui.js`, `ui/trinity.css` and the shared JSON to be vendored under `launcher/src/trinity/`. The sync script copies them.

The adapter adds one page and one navigation entry. It does not change the existing Play, Setup, Settings or About pages.

## What would count as verified

1. Compile the launcher with the page.
2. Build a runtime from the API 4 source.
3. Load `trinity.dll` from a prepared mods folder.
4. Capture the log and the panel.
5. Record the exact host version and API version.
6. Repeat with a plugin that has an observable non-gameplay effect, then a gameplay effect only after a gameplay interface exists.

None of those steps has been completed for this host.
