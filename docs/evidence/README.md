# Test evidence

These files are test evidence, not SDK inputs. The older runtime logs and
in-game captures come from the EmblemWeave development build at
`EmblemWeave/engine/fefw-recomp/game-build/fefw/build/release/fefw.exe`. The
25 November 2026 drop-in runs used the installed EmblemWeave 0.1.43 runtime at
`E:\EmblemWeave`, which loads code mods. The three launcher images come from
actual launcher windows, not a browser mock or a dev-server render.

## Files

| File | What it shows |
| --- | --- |
| `emblemweave-trinity.png` | The native Trinity page in the installed EmblemWeave 0.1.43 launcher at `E:\EmblemWeave`, captured from its own rendered frame with `--page Trinity --screenshot` after a `plugins\Trinity.dll` drop-in was staged. 1440 x 900. |
| `emblemweave-f1-menu.png` | The host's own F1 menu, before selecting the plugin page. |
| `emblemweave-mod-panel.png` | F1 > Advanced > Mods with the NXP demo panel. The host name and id are correct, and the Enabled and Speed controls are visible. |
| `emblemweave-runtime.log` | The host log from a run that loaded the demo, registered its panel, started the code mod and later recorded a settings change. |
| `trinity-core-runtime.log` | An excerpt from the 25 November 2026 run that loaded Trinity Core, identified `emblemweave` / `nxmod-api-4`, registered the Trinity panel and ran the prefix classifier. It records 0.1.0; the combined 0.2.0 run is below. |
| `mod-compat-runtime.log` | The 0.2.0 headless load/init run on API 4 that loaded the original Turn Control 0.3.0 and Quicksave 1.0.0 binaries alongside Trinity, with all three ready messages. |
| `fefw-runtime.log` | The prepared fefw-recomp 0.2.13 runtime on the supplied 1.1.1 dump, with the original Quicksave, Turn Control and Trinity 0.2.0 loaded on API 3. |
| `obelisk-runtime.log` | The prepared ObeliskChamber 0.3.2 runtime on the same dump, with the original Quicksave, Turn Control and Trinity 0.2.0 loaded on API 5; panel registration and the first frame are recorded. |
| `emblemweave-dropin-runtime.log` | The installed EmblemWeave 0.1.43 runtime after staging a `plugins\Trinity.dll` drop-in; load, host identity, panel registration and prefix check are recorded. |
| `fefw-dropin-runtime.log` | fefw-recomp 0.2.13 after staging a nested `plugins\TrinityCore\exefs\Trinity.dll`; API 3 headless load is recorded. |
| `obelisk-dropin-runtime.log` | ObeliskChamber 0.3.2 after staging a flat `plugins\Trinity.dll`; API 5 panel registration is recorded. |
| `x-emblemweave-dropin-runtime.log` | The second EmblemWeave 0.1.43 install at `X:\EmblemWeave`, run with a generated valid INI and the root `Trinity.dll` already staged; API 4 load, host identity and panel registration are recorded. |
| `launcher-plugin-staging.md` | The root/`plugins` staging matrix, stale hashes, staged hashes and staged paths for all three installed launchers. |
| `fefw-shutdown-124.md` | Timing and source analysis of `--max-seconds`: the `124` status is the runtime's deliberate deadline exit, not a 120-second hang, and does not involve shader-cache or Trinity shutdown. |
| `linux-host.txt` | The native Linux x64 build, ELF/export checks and `dlopen` harness result. |
| `launcher-fefw-trinity.png` | The actual fefw-recomp 0.2.13 Tauri window with the Trinity page open. Built from the `v0.2.13` tag with the shared surface. Captured through WebView2 CDP at 1180 x 1000. |
| `launcher-obelisk-trinity.png` | The actual ObeliskChamber 0.3.2 Tauri window with the Trinity page open. Built from the current 0.3.2 source with the shared surface. Captured through WebView2 CDP at 1180 x 1000. |

The three launcher images show the supplied gear-and-ribbon logo at the top of
the page. The old labels below each launcher name were removed; the launcher
name and its accent color carry the identity instead.

## How the launcher captures were made

The shared catalog, prefix registry, UI, CSS and logo were synced into each
launcher source. Each launcher received one page module and one navigation
entry. No existing page was replaced.

- **EmblemWeave**: the installed 0.1.43 launcher at `E:\EmblemWeave` was
  rebuilt from the current host tree. `plugins\Trinity.dll` was supplied as a
  drop-in and staged by the launcher. It was started from `E:\EmblemWeave` with
  `--page Trinity --screenshot`. The built-in capture path reads the launcher's
  own GPU frame after the page has settled, so no other window can appear in the
  image.
- **fefw-recomp**: the `v0.2.13` tag was checked out in a separate worktree,
  integrated, and built with `npm run tauri build -- --no-bundle`. The resulting
  `fefw-launcher.exe` was installed in `X:\fefw-recomp-0.2.13`, launched with
  WebView2 remote debugging enabled, navigated to `[data-page="trinity"]`, and
  captured through CDP at 1180 x 1000. This is the installed 0.2.13 launcher,
  not the newer 0.3.0 source.
- **ObeliskChamber**: the current 0.3.2 launcher source was integrated and built
  the same way. The resulting `ObeliskChamber-launcher.exe` was installed in
  `X:\ObeliskChamber-0.3.2`, launched with a dedicated WebView2 user-data
  folder and remote debugging, navigated to `[data-page="trinity"]`, and
  captured through CDP at 1180 x 1000.

The source changes in the upstream launcher checkouts remain local. Nothing was
pushed to either host repository.

## Current status

The launcher windows verify that the Trinity page loads, render the catalog and
read the prefix registry in all three hosts.

The root/`plugins` drop-in path was verified separately on all three installed
launchers. Each launcher restored the 0.2.0 payload from a root DLL and from a
`plugins/` DLL (flat or nested package), and each runtime then loaded the staged
copy. The stale-marker and hash matrix is in
[launcher-plugin-staging.md](launcher-plugin-staging.md).

The second EmblemWeave install at `X:\EmblemWeave` had no game INI. A valid INI
was generated from the working `E:\EmblemWeave` configuration, keeping its
exefs and ROMFS dump paths and pointing mods, saves and work folders at X. The
X runtime then loaded Trinity Core 0.2.0 on API 4 and registered its panel. Its
shader cache was empty, so the run skipped translated draws and is not a visual
result.

The same Windows x64 `Trinity.dll` (SHA-256 `f873124e...bd81cfd`) loaded in all
three game runtimes:

- **EmblemWeave, API 4:** headless load/init with the original Quicksave 1.0.0
  and Turn Control 0.3.0; Trinity identified the host, registered its panel and
  classified the prefix set. The drop-in run used the installed 0.1.43 runtime
  at `E:\EmblemWeave`; the NXP demo panel was rendered in a separate windowed
  development run.
- **fefw-recomp 0.2.13, API 3:** headless load/init with the same two mods.
  API 3 has no panel slot, so Trinity correctly ran headless. The first frame
  was reached.
- **ObeliskChamber 0.3.2, API 5:** headless load/init with the same two mods.
  Trinity registered its panel and Turn Control registered its list screen. The
  first frame was reached; nothing was drawn or interacted with.

This is initialization evidence, not gameplay. The Trinity 0.2.0 panel has not
yet been captured in a windowed run in any host. The Linux harness is a native
load result, not a Linux game launch.

## Runtime lines

The EmblemWeave drop-in runtime lines were:

```text
mods: TrinityCore/Trinity.dll: loaded, starts at the first frame
mods: TrinityCore/Trinity.dll: Trinity Core 0.2.0 on emblemweave
      (nxmod-api-4), host prefix emblemweave., capabilities 0x0000007f
mods: TrinityCore/Trinity.dll: panel registered
mods: TrinityCore/Trinity.dll: code mod started (0 hooks)
```

The second EmblemWeave install at `X:\EmblemWeave` produced the same load and
host lines; its excerpt also records the empty shader cache warning:

```text
mods: TrinityCore/Trinity.dll: loaded, starts at the first frame
mods: TrinityCore/Trinity.dll: Trinity Core 0.2.0 on emblemweave
      (nxmod-api-4), host prefix emblemweave., capabilities 0x0000007f
mods: TrinityCore/Trinity.dll: panel registered
mods: TrinityCore/Trinity.dll: code mod started (0 hooks)
```

The older demo-panel log lines were:

```text
mods: NXPluginDemo/nxp_demo.dll: loaded, starts at the first frame
mods: NXPluginDemo/nxp_demo.dll: Unified Plugin Demo 0.1.0 on emblemweave
      (nxmod-api-4), capabilities 0x0000007f
mods: NXPluginDemo/nxp_demo.dll: panel registered with the host
mods: NXPluginDemo/nxp_demo.dll: code mod started (0 hooks)
```

After the checkbox and slider were clicked, the settings file was read back as
`demo.enabled=0` and `demo.speed=107`, down from `demo.enabled=1` and
`demo.speed=100`. It was reset to defaults after the test. The captured runtime
log is from a load/panel run, not from the click run, so it does not contain the
`disabled` line.

The ObeliskChamber panel registration lines were:

```text
mods: TrinityCore/Trinity.dll: Trinity Core 0.2.0 on obelisk-chamber (nxmod-api-5), host prefix obeliskchamber., capabilities 0x0000007f
mods: TrinityCore/Trinity.dll: added the panel "Trinity" to the Mods window
mods: TrinityCore/Trinity.dll: panel registered
```

The run was headless, so these lines prove registration, not drawing.

## How the runtime test was run

The development executable was started with an INI pointing at the game dump,
the EmblemWeave mods folder and a separate save directory. The demo DLL was
installed at:

```text
%APPDATA%\EmblemWeave\play\mods\NXPluginDemo\exefs\nxp_demo.dll
```

The F1 menu was opened, Advanced was selected, and Mods was opened. The window
was captured with the Windows desktop API.

The fefw-recomp and ObeliskChamber runs used the supplied 1.1.1 dump, each
host's own prepared runtime, a separate save directory and `--headless
--max-seconds 25`. The exact logs and limits are in `fefw-runtime.log` and
`obelisk-runtime.log`.

The drop-in runs on 25 November 2026 used each host's installed runtime with
`--headless --max-seconds 5` and its own INI. The deadline ended the process
with the runtime's usual non-zero `--max-seconds` status; the Trinity lines
were already recorded. The X:\EmblemWeave run used a generated INI and
`--max-seconds 10`; it reused the E: exefs and ROMFS dump paths because the X
install has no game files of its own. The excerpts are in the four
`*-dropin-runtime.log` files.

## Trinity Core load

The reference plugin was built from this repository and installed as:

```text
%APPDATA%\EmblemWeave\play\mods\TrinityCore\exefs\Trinity.dll
%APPDATA%\EmblemWeave\play\mods\TrinityCore\plugin.json
```

The 0.2.0 development runtime loaded it alongside the original Quicksave and
Turn Control binaries. `mod-compat-runtime.log` records all three ready
lines, the host identity, panel registration and prefix check. The plugin
folder contained `trinity.enabled=1` and `trinity.prefix_guard=1` after the run.

The drop-in run used the installed launcher's staged copy instead:

```text
E:\EmblemWeave\data\play\mods\TrinityCore\exefs\Trinity.dll
E:\EmblemWeave\data\play\mods\TrinityCore\plugin.json
```

This was a headless load/init run. It proves loading, independent initialization
and registration; it does not prove that the panel was drawn or that either mod
changed a save, turn or facility. A windowed capture of the in-game Trinity
panel is still pending.

## Limits

- These are Windows x64 builds on one host. The Linux `.so` is loaded by a
  native harness, not a Linux game runtime.
- The EmblemWeave page capture is a launcher page, not the NXP panel inside the
  game.
- All three launcher windows are verified. Game runtime loading is verified
  headless on EmblemWeave API 4, fefw-recomp API 3 and ObeliskChamber API 5,
  including both installed EmblemWeave runtimes after a launcher-staged drop-in.
- Root and `plugins/` discovery is verified for each launcher; the launcher
  pages themselves do not display the staged path, so the staging evidence is
  the hash matrix and runtime logs, not a screenshot.
- The X:\EmblemWeave shader cache was empty, so its headless run skipped
  translated draws. That does not affect the load/init result, and it is not a
  visual or gameplay result.
- The Trinity 0.2.0 panel is registered in EmblemWeave and ObeliskChamber but
  has not been drawn or interacted with. fefw-recomp API 3 has no panel slot.
- No save, load, turn, facility or gameplay action was exercised in any host.
- The upstream launcher source changes are local and unmerged.
- The images do not prove gameplay behavior.
