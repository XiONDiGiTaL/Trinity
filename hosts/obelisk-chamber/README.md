# ObeliskChamber host adapter

## Current state

- Packaged release inspected: `X:\ObeliskChamber-0.3.2`.
- `NXMOD_API_VERSION` 5.
- `nxmod_init`, `nxmod_early` and `add_panel` strings are present.
- The member after `free_mem` is `open_list` in this host and `add_panel` in EmblemWeave. The runtime distinguishes them by the first argument.
- The exact packaged table layout was not probed member by member.
- A prepared 0.3.2 game runtime loaded the same `Trinity.dll` headless on API 5.
  Trinity reported `0.2.0 on obelisk-chamber (nxmod-api-5)`, registered its
  panel and reached the first frame. The panel was not drawn or interacted
  with. Evidence: [../../docs/evidence/obelisk-runtime.log](../../docs/evidence/obelisk-runtime.log).
- The release installer staged Trinity 0.2.0 at
  `X:\ObeliskChamber-0.3.2\mods\TrinityCore`; the runtime run above loaded that
  payload.
- Upstream source is newer than the package. The launcher and API source were inspected, but that is not the packaged binary.

## Adapter plan

1. Resolve the `open_list`/`add_panel` slot explicitly. Do not use the API version alone.
2. Expose panels through `nxp.ui`.
3. Expose the list screen through a host interface such as `obeliskchamber.list` version 1.
4. Keep the old `open_list` behaviour for existing mods.
5. Pass the true host id, `obelisk-chamber`.
6. Test the same `Trinity.dll` that EmblemWeave loads.
7. Record the exact table layout and runtime version in `docs/COMPATIBILITY.md`.

Step 6 is now satisfied at the load/init and panel-registration level. The
remaining adapter work is steps 1 to 4: make the `open_list`/`add_panel` split
explicit and expose the list screen through its own host interface.

## Official release install

```powershell
python tools/install_trinity.py --host-dir "X:\ObeliskChamber-0.3.2" --version 0.2.0
```

The helper verifies the official archive against `SHA256SUMS` and replaces only `<host>\mods\TrinityCore`. The packaged API 5 runtime loaded this payload and logged the panel registration; the slot was not probed member by member, and the panel was not drawn. See [../../docs/COMPATIBILITY.md](../../docs/COMPATIBILITY.md).

For a bundled host release, stage the verified payload with `--bundle vendor/trinity`, copy `TrinityCore` into the host's `mods` folder, and compare `plugin.json` on first run. Keep the version and checksum pinned. See [docs/INSTALL.md](../../docs/INSTALL.md).

## Launcher

`hosts/launchers/obelisk/trinity.js` is the Tauri page adapter. ObeliskChamber treats non-title screens as framed panels, so the shared surface switches to sharp corners and the host's gold treatment through `data-trinity-host="obelisk-chamber"`.

The adapter does not add a second navigation system. It uses the launcher's existing `PAGES` map and controller navigation.

## Slot rule

A call with a `Ctx` first argument is `open_list`. A call with a title is `add_panel`. The compatibility adapter always passes a title. A native adapter should make the distinction explicit in its own code and tests rather than rely on the argument's type at a distance.
