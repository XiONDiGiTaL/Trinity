# ObeliskChamber host adapter

## Current state

- Packaged release inspected: `X:\ObeliskChamber-0.3.2`.
- `NXMOD_API_VERSION` 5.
- `nxmod_init`, `nxmod_early` and `add_panel` strings are present.
- The member after `free_mem` is `open_list` in this host and `add_panel` in EmblemWeave. The runtime distinguishes them by the first argument.
- The exact packaged table layout was not runtime-probed.
- No prepared game was available, so the Trinity DLL was not loaded.
- Upstream source is newer than the package. The launcher and API source were inspected, but that is not the packaged binary.

## Adapter plan

1. Resolve the `open_list`/`add_panel` slot explicitly. Do not use the API version alone.
2. Expose panels through `nxp.ui`.
3. Expose the list screen through a host interface such as `obeliskchamber.list` version 1.
4. Keep the old `open_list` behaviour for existing mods.
5. Pass the true host id, `obelisk-chamber`.
6. Test the same `trinity.dll` that EmblemWeave loads.
7. Record the exact table layout and runtime version in `docs/COMPATIBILITY.md`.

Until step 6, the matrix says "not loaded". The source string match is not runtime evidence.

## Launcher

`hosts/launchers/obelisk/trinity.js` is the Tauri page adapter. ObeliskChamber treats non-title screens as framed panels, so the shared surface switches to sharp corners and the host's gold treatment through `data-trinity-host="obelisk-chamber"`.

The adapter does not add a second navigation system. It uses the launcher's existing `PAGES` map and controller navigation.

## Slot rule

A call with a `Ctx` first argument is `open_list`. A call with a title is `add_panel`. The compatibility adapter always passes a title. A native adapter should make the distinction explicit in its own code and tests rather than rely on the argument's type at a distance.
