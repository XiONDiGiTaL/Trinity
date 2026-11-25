# fefw-recomp host adapter

## Current state

- Packaged release inspected: `X:\fefw-recomp-0.2.13`.
- `NXMOD_API_VERSION` 3 in that package.
- `nxmod_init` and `nxmod_early` are present.
- `add_panel` is absent, so the packaged host cannot show an NXP panel.
- `on_frame`, file APIs, hook sites and `call_guest` follow the shared lineage.
- Upstream `master` is newer than the package and is the basis for the launcher source inspection. It is not proof that the packaged 0.2.13 binary contains the same code.
- A prepared game was built from the supplied update-131072 dump with
  `fefw-launcher.exe --prepare --game E:\FEFW`.
- On 25 November 2026 the headless runtime loaded the original Quicksave 1.0.0,
  Turn Control 0.3.0 and Trinity 0.2.0 together. All three started; Trinity
  identified `fefw-recomp` / `nxmod-api-3` and stayed headless because API 3 has
  no `add_panel`.
- Evidence: [../../docs/evidence/fefw-runtime.log](../../docs/evidence/fefw-runtime.log).
  Quicksave reported that its save folder was not found in the headless profile;
  no save, turn or panel action was performed. The runtime log reached
  `max_seconds` and ended with the runtime's deliberate `124` deadline status;
  see [../../docs/evidence/fefw-shutdown-124.md](../../docs/evidence/fefw-shutdown-124.md).

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
3. Load `Trinity.dll` from a prepared mods folder.
4. Capture the log and the panel.
5. Record the exact host version and API version.
6. Repeat with a plugin that has an observable non-gameplay effect, then a gameplay effect only after a gameplay interface exists.

Runtime loading is now verified on the packaged API 3 host. The panel and gameplay
steps still require an API 4+ runtime; API 3 cannot show an NXP panel.

## Official release install

Use the checked downloader from the Trinity checkout or a release copy:

```powershell
python tools/install_trinity.py --host-dir "X:\fefw-recomp-0.2.13" --version 0.2.0
```

The helper downloads the official archive and `SHA256SUMS`, verifies the hash before extraction, and replaces only `<host>\mods\TrinityCore`. On this host's API 3 table the plugin runs headless: settings, files and frame work, but no panel.

For a host release that must work offline, vendor a verified payload with `--bundle vendor/trinity`, then copy `TrinityCore` into the release's `mods` folder and compare `plugin.json` on first run. Pin the version and checksum; do not download a floating `latest` at startup. See [docs/INSTALL.md](../../docs/INSTALL.md).
