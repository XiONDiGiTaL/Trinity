# ObeliskChamber host adapter

## Current state

- Packaged release inspected: `X:\ObeliskChamber-0.3.2`.
- `NXMOD_API_VERSION` 5.
- `nxmod_init`, `nxmod_early` and `add_panel` strings are present.
- The member after `free_mem` is `open_list` in this host and `add_panel` in EmblemWeave. The runtime distinguishes them by the first argument.
- The exact packaged table layout was not runtime-probed.
- No prepared game was available, so the Trinity DLL was not loaded.
- The release installer staged Trinity 0.2.0 at
  `X:\ObeliskChamber-0.3.2\mods\TrinityCore`; this is an installation check,
  not a runtime claim.
- Upstream source is newer than the package. The launcher and API source were inspected, but that is not the packaged binary.

## Adapter plan

1. Resolve the `open_list`/`add_panel` slot explicitly. Do not use the API version alone.
2. Expose panels through `nxp.ui`.
3. Expose the list screen through a host interface such as `obeliskchamber.list` version 1.
4. Keep the old `open_list` behaviour for existing mods.
5. Pass the true host id, `obelisk-chamber`.
6. Test the same `Trinity.dll` that EmblemWeave loads.
7. Record the exact table layout and runtime version in `docs/COMPATIBILITY.md`.

Until step 6, the matrix says "not loaded". The source string match is not runtime evidence.

## Official release install

```powershell
python tools/install_trinity.py --host-dir "X:\ObeliskChamber-0.3.2" --version 0.2.0
```

The helper verifies the official archive against `SHA256SUMS` and replaces only `<host>\mods\TrinityCore`. The packaged API 5 table's `open_list`/`add_panel` slot was not runtime-probed; do not claim a panel until step 6 above is done.

For a bundled host release, stage the verified payload with `--bundle vendor/trinity`, copy `TrinityCore` into the host's `mods` folder, and compare `plugin.json` on first run. Keep the version and checksum pinned. See [docs/INSTALL.md](../../docs/INSTALL.md).

## Launcher

`hosts/launchers/obelisk/trinity.js` is the Tauri page adapter. ObeliskChamber treats non-title screens as framed panels, so the shared surface switches to sharp corners and the host's gold treatment through `data-trinity-host="obelisk-chamber"`.

The adapter does not add a second navigation system. It uses the launcher's existing `PAGES` map and controller navigation.

## Slot rule

A call with a `Ctx` first argument is `open_list`. A call with a title is `add_panel`. The compatibility adapter always passes a title. A native adapter should make the distinction explicit in its own code and tests rather than rely on the argument's type at a distance.
