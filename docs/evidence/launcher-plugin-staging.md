# Launcher drop-in staging test

Date: 25 November 2026.

All three launchers were rebuilt with the prepared discovery module and
installed over their tested copies. A stale marker was written to the staged
`TrinityCore/exefs/Trinity.dll` before each launch, so the matching hash after
launch proves that the launcher copied the drop-in payload.

| Host | Install | Drop-in source | Staged destination |
| --- | --- | --- | --- |
| EmblemWeave 0.1.43 | `E:\EmblemWeave` | root `Trinity.dll`, then `plugins\Trinity.dll` | `data\play\mods\TrinityCore\exefs\Trinity.dll` |
| EmblemWeave 0.1.43 | `X:\EmblemWeave` | root `Trinity.dll` | `data\play\mods\TrinityCore\exefs\Trinity.dll` |
| fefw-recomp 0.2.13 | `X:\fefw-recomp-0.2.13` | root `Trinity.dll`, then `plugins\TrinityCore\exefs\Trinity.dll` | `mods\TrinityCore\exefs\Trinity.dll` |
| ObeliskChamber 0.3.2 | `X:\ObeliskChamber-0.3.2` | root `Trinity.dll`, then `plugins\Trinity.dll` | `mods\TrinityCore\exefs\Trinity.dll` |

The stale marker hashes were
`08bb9b939714efa3d3cb378b470ecd0d85ac6567235740388f76ed65ab1606a8`.
After every launch, all three staged DLLs matched
`f873124ed90671c961decda9bcd2678269de47ec614379044e89a22e9bd81cfd`
(Trinity 0.2.0), and `plugin.json` reported version `0.2.0`.

| Run | Result |
| --- | --- |
| EmblemWeave, root DLL | Staged from `E:\EmblemWeave\Trinity.dll`; launcher screenshot captured |
| EmblemWeave, `plugins\Trinity.dll` | Staged from the flat plugin; screenshot captured |
| fefw-recomp, root DLL | Staged from `X:\fefw-recomp-0.2.13\Trinity.dll` |
| fefw-recomp, nested package | Staged from `plugins\TrinityCore\exefs\Trinity.dll` |
| ObeliskChamber, root DLL | Staged from `X:\ObeliskChamber-0.3.2\Trinity.dll` |
| ObeliskChamber, flat plugin | Staged from `plugins\Trinity.dll` |
| EmblemWeave at `X:\EmblemWeave`, root DLL | Staged from `X:\EmblemWeave\Trinity.dll`; launcher screenshot captured |

After staging, each game runtime was started headless with
`--max-seconds 5` and its own INI. Runtime excerpts are in
`emblemweave-dropin-runtime.log`, `fefw-dropin-runtime.log` and
`obelisk-dropin-runtime.log`. They show the expected host identity:

```text
Trinity Core 0.2.0 on emblemweave (nxmod-api-4)
Trinity Core 0.2.0 on fefw-recomp (nxmod-api-3)
Trinity Core 0.2.0 on obelisk-chamber (nxmod-api-5)
```

The existing `.nxp-settings.ini` files in the fefw-recomp and ObeliskChamber
mod folders remained in place. This is staging, load and initialization
evidence; the runs were headless, so they do not prove panel drawing or
gameplay.
