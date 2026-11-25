# Test evidence

These files are test evidence, not SDK inputs. The runtime logs and in-game
captures come from the EmblemWeave development build at:

`EmblemWeave/engine/fefw-recomp/game-build/fefw/build/release/fefw.exe`

The installed game runtime is older and does not load code mods. The three
Launcher images in this folder come from actual launcher windows, not a browser
mock or a dev-server render.

## Files

| File | What it shows |
| --- | --- |
| `emblemweave-trinity.png` | The native Trinity page in the EmblemWeave 0.1.43 release launcher, captured from its own rendered frame with `--page Trinity --screenshot`. 1440 x 900. |
| `emblemweave-f1-menu.png` | The host's own F1 menu, before selecting the plugin page. |
| `emblemweave-mod-panel.png` | F1 > Advanced > Mods with the NXP demo panel. The host name and id are correct, and the Enabled and Speed controls are visible. |
| `emblemweave-runtime.log` | The host log from a run that loaded the demo, registered its panel, started the code mod and later recorded a settings change. |
| `trinity-core-runtime.log` | An excerpt from the 25 November 2026 run that loaded Trinity Core, identified `emblemweave` / `nxmod-api-4`, registered the Trinity panel and ran the prefix classifier. It records 0.1.0; the combined 0.2.0 run is below. |
| `mod-compat-runtime.log` | The 0.2.0 run that loaded the original Quick Save 1.0.0 and Turn Control 0.3.0 binaries alongside Trinity, with all three ready messages. |
| `linux-host.txt` | The native Linux x64 build, ELF/export checks and `dlopen` harness result. |
| `launcher-fefw-trinity.png` | The actual fefw-recomp 0.2.13 Tauri window with the Trinity page open. Built from the `v0.2.13` tag with the shared surface. Captured with the Windows window API at 1180 x 1000. |
| `launcher-obelisk-trinity.png` | The actual ObeliskChamber 0.3.2 Tauri window with the Trinity page open. Built from the current 0.3.2 source with the shared surface. Captured with the Windows window API at 1180 x 1000. |

The three launcher images show the supplied gear-and-ribbon logo at the top of
the page. The old labels below each launcher name were removed; the launcher
name and its accent color carry the identity instead.

## How the launcher captures were made

The shared catalog, prefix registry, UI, CSS and logo were synced into each
launcher source. Each launcher received one page module and one navigation
entry. No existing page was replaced.

- **EmblemWeave**: the release launcher was built from the current host tree.
  It was started from `X:\EmblemWeave` with `--page Trinity --screenshot`. The
  built-in capture path reads the launcher's own GPU frame after the page has
  settled, so no other window can appear in the image.
- **fefw-recomp**: the `v0.2.13` tag was checked out in a separate worktree,
  integrated, and built with `npm run tauri build -- --no-bundle`. The resulting
  `fefw-launcher.exe` was installed in `X:\fefw-recomp-0.2.13`, launched with
  WebView2 remote debugging enabled, navigated to Trinity, and captured with the
  Windows window API at 1180 x 1000. This is the installed 0.2.13 launcher, not
  the newer 0.3.0 source.
- **ObeliskChamber**: the current 0.3.2 launcher source was integrated and built
  the same way. The resulting `ObeliskChamber-launcher.exe` was installed in
  `X:\ObeliskChamber-0.3.2`, launched, navigated to Trinity, and captured at
  1180 x 1000.

The source changes in the upstream launcher checkouts remain local. Nothing was
pushed to either host repository.

## Current status

The launcher windows verify that the Trinity page loads, render the catalog and
read the prefix registry in all three hosts. They do not verify the NXP runtime
DLL in fefw-recomp or ObeliskChamber. Only EmblemWeave has runtime evidence for
the NXP plugin.

The fefw-recomp and ObeliskChamber cards report `Launcher verified`. The
EmblemWeave card keeps `Runtime verified` because its plugin was loaded by the
game runtime. Trinity 0.2.0 also loaded with Quick Save 1.0.0 and Turn Control
0.3.0; this is initialization evidence, not gameplay. The Linux harness is a
native load result, not a Linux game launch.

## Runtime lines

The relevant EmblemWeave log lines are:

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

## How the runtime test was run

The development executable was started with an INI pointing at the game dump,
the EmblemWeave mods folder and a separate save directory. The demo DLL was
installed at:

```text
%APPDATA%\EmblemWeave\play\mods\NXPluginDemo\exefs\nxp_demo.dll
```

The F1 menu was opened, Advanced was selected, and Mods was opened. The window
was captured with the Windows desktop API.

## Trinity Core load

The reference plugin was built from this repository and installed as:

```text
%APPDATA%\EmblemWeave\play\mods\TrinityCore\exefs\Trinity.dll
%APPDATA%\EmblemWeave\play\mods\TrinityCore\plugin.json
```

The 0.2.0 development runtime loaded it alongside the original Quick Save and
Turn Control binaries. `mod-compat-runtime.log` records all three ready
lines, the host identity, panel registration and prefix check. The plugin
folder contained `trinity.enabled=1` and `trinity.prefix_guard=1` after the run.

This was a headless run. It proves loading, independent initialization and
registration; it does not prove that the panel was drawn or that either mod
changed a save, turn or facility. A windowed capture of the in-game Trinity
panel is still pending.

## Limits

- This is one Windows x64 build.
- The EmblemWeave page capture is a launcher page, not the NXP panel inside the
  game.
- fefw-recomp and ObeliskChamber launcher windows are verified; their game
  runtimes and in-game NXP panels were not run.
- The same DLL is runtime-verified only in EmblemWeave. It is not proven to load
  in the other two game runtimes.
- The upstream launcher source changes are local and unmerged.
- The images do not prove gameplay behavior.
- The Linux `.so` is loaded by a native harness, not a Linux game runtime; no
  Linux launcher is available in this workspace.
