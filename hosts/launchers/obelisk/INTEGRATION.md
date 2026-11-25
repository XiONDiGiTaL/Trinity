# ObeliskChamber launcher integration

The adapter is `trinity.js`. It mounts the shared surface with the host id
`obelisk-chamber`. ObeliskChamber treats every non-title screen as a framed
panel, so the Trinity page drops the soft radii and uses the host's sharper
frame and gold rules.

## Apply to an upstream checkout

Tested against ObeliskChamber launcher 0.3.2 at commit
`dc0b316452dbdc621525e28a9f28cf0cb2c15148`.

From the Trinity repository root:

```powershell
git -C ../ObeliskChamber apply integrations/patches/obelisk-launcher.patch
powershell -NoProfile -ExecutionPolicy Bypass -File integrations/sync-trinity.ps1 -ObeliskRoot ../ObeliskChamber
cd ../ObeliskChamber/launcher
npm ci
npm run build
```

The patch adds the CSS import, the page import, the `PAGES` entry, the
`nav.trinity` key and the `nav.trinityHelp` key. The sync script copies the
shared files into `launcher/src/trinity/` and the adapter into
`launcher/src/pages/trinity.js`.

The host shows navigation only on the title screen. Open Trinity from that
navigation, or press Escape from a panel screen to return to the title first.

## What was verified

`npm run build` passed and the built page rendered in the launcher shell with a
mock Tauri bridge. The screenshot is
`docs/evidence/launcher-obelisk-trinity.png`. The Tauri window was not run, the
change is not merged upstream, and the game runtime was not loaded.

## Rollback

Remove the `trinity` entry from `PAGES`, its import, the CSS import and both
locale keys, then delete `launcher/src/pages/trinity.js` and
`launcher/src/trinity/`.
