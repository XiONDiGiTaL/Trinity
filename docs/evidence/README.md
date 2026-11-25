# EmblemWeave runtime evidence

These files are test evidence, not SDK inputs. They come from the EmblemWeave
development build at:

`EmblemWeave/engine/fefw-recomp/game-build/fefw/build/release/fefw.exe`

The installed `%APPDATA%\EmblemWeave\play\fefw.exe` is older and does not load
code mods.

## Files

| File | What it shows |
| --- | --- |
| `emblemweave-f1-menu.png` | The host's own F1 menu, before selecting the plugin page. |
| `emblemweave-mod-panel.png` | F1 > Advanced > Mods with the NXP demo panel. The host name and id are correct, and the Enabled and Speed controls are visible. |
| `emblemweave-runtime.log` | The host log from a run that loaded the demo, registered its panel, started the code mod and later recorded a settings change. |
| `trinity-core-runtime.log` | An excerpt from the 25 November 2026 run that loaded Trinity Core, identified `emblemweave` / `nxmod-api-4`, registered the Trinity panel and ran the prefix classifier. |

The panel capture is from the build immediately before the final descriptor
size macro was added. That macro only changes the init-time size check from
`sizeof(NxpPluginDesc)` to `NXP_PLUGIN_DESC_V1_SIZE`, which is the same value.
The UI path is unchanged. CTest passes on the final build.

## Runtime lines

The relevant log lines are:

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

## How the test was run

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
%APPDATA%\EmblemWeave\play\mods\TrinityCore\exefs\trinity.dll
%APPDATA%\EmblemWeave\play\mods\TrinityCore\plugin.json
```

The development runtime loaded it alongside the older demo. The excerpt records
the host identity, panel registration and prefix check. The plugin folder
contained `trinity.enabled=1` and `trinity.prefix_guard=1` after the run.

This was a headless run. It proves loading and registration, not that the panel
was drawn. A windowed capture of the Trinity panel is still pending.

## Limits

- This is one host and one Windows x64 build.
- The panel was rendered and its controls were clicked. No gameplay value was
  changed.
- fefw-recomp 0.2.13 and ObeliskChamber 0.3.2 were not run.
- The images do not prove that the same DLL works in the other two hosts.
