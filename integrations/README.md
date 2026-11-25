# Vendoring the shared launcher surface

`trinity-ui.js`, `trinity.css`, `prefix-core.mjs`, `catalog.json` and
`prefixes.json` are kept in `Trinity/shared` and `Trinity/launcher`. The two
Tauri launchers vendor them under `launcher/src/trinity/` so a release build
does not read outside its own project root.

From the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Trinity/integrations/sync-trinity.ps1
```

The script takes an optional `-FefwRoot` and `-ObeliskRoot`; by default it looks
for sibling checkouts named `fefw-recomp` and `ObeliskChamber`. It copies only
the shared files and the host adapter, then prints the changed paths. The
checked-in host patch still owns the two navigation entries and the locale
keys, so re-running the sync cannot silently add a second page.

The EmblemWeave adapter is native Rust and embeds the same JSON at compile time,
so it has no runtime dependency on this folder.
