# fefw-recomp launcher integration

The adapter is `trinity.js`. It mounts the shared surface with the host id
`fefw-recomp`, so the page keeps the launcher's cool surface and soft corners.

## Apply to an upstream checkout

Tested against fefw-recomp launcher 0.2.13 at tag `v0.2.13`, commit
`bab923d981d867c72883be132410f9fc16d17c67`. The same patch applies to the
current 0.3.0 source because the patched lines are unchanged.

From the Trinity repository root:

```powershell
git -C ../fefw-recomp apply integrations/patches/fefw-launcher.patch
powershell -NoProfile -ExecutionPolicy Bypass -File integrations/sync-trinity.ps1 -FefwRoot ../fefw-recomp
cd ../fefw-recomp/launcher
npm ci
npm run tauri build -- --no-bundle
```

The patch adds the CSS import, the page import, the `PAGES` entry and the
`nav.trinity` locale key. The sync script copies the shared files into
`launcher/src/trinity/` and the adapter into `launcher/src/pages/trinity.js`.
It does not edit the host's existing pages.

The patch is authoritative for the host-side edits. If the host changes the
same lines, apply the changes by hand rather than forcing the patch.

## What was verified

`npm run tauri build -- --no-bundle` passed. The resulting 0.2.13 executable
was installed in `X:\fefw-recomp-0.2.13`, launched, and its actual Tauri window
opened the Trinity page. The screenshot is
`docs/evidence/launcher-fefw-trinity.png`. The change is not merged upstream,
and the game runtime was not loaded.

## Rollback

Remove the `trinity` entry from `PAGES`, its import, the CSS import and the
locale key, then delete `launcher/src/pages/trinity.js` and
`launcher/src/trinity/`.
