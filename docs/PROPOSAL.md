# Draft proposal: a shared NXP plugin ABI for all three ports

Status: draft for maintainer review. Not posted. Not accepted.

> Historical proposal, migrated from the predecessor `unified-plugin-sdk` workspace on 25 November 2026. The SDK now lives in this repository under `include/`, `hosts/` and `plugins/`. See [NXP.md](NXP.md) for current paths and [COMPATIBILITY.md](COMPATIBILITY.md) for verified status.

This document is a proposal, not a claim that any maintainer has agreed to it.
It summarizes the phase 0 work and the smallest next steps that would make one
plugin work across EmblemWeave, fefw-recomp and ObeliskChamber.

## Problem

The three ports already share a code-mod lineage:

- a DLL in a mod's `exefs` folder;
- `nxmod_init` at the first frame, optional `nxmod_early` at load;
- the same three hook-site kinds and `.sites` syntax;
- `call_guest`, guest memory access and file edits;
- no unload;
- a per-mod `NxModApi` table with a version and size.

They do not share a plugin lifecycle, metadata, capability discovery, UI
surface, settings service, file scoping, launcher manifest or diagnostics. A
developer who wants a plugin on all three ports currently maintains three
codebases, or targets the lowest common API and loses panels and settings.

## What was built

A separate SDK, now consolidated into this repository (originally `unified-plugin-sdk/`):

- `include/nxp/nxp.h`: a versioned C ABI.
- `nxp_get_plugin`: the native entry point.
- `NxpHostApi`: host identity, logging and `get_interface`.
- Optional interfaces: `nxp.ui`, `nxp.settings`, `nxp.files`, `nxp.frame`.
- A compatibility adapter that exports `nxmod_init` and maps the current
  `NxModApi` onto NXP.
- A demo plugin with metadata, host discovery, a persisted setting, a native
  panel, a frame callback and logging.
- A standalone host harness.

The demo was built with clang-cl 23.1.5 and lld-link. CTest passes 1 of 1. It
loaded in EmblemWeave's development build, identified the host as
`emblemweave` with `nxmod-api-4`, registered a panel, rendered it in F1 >
Advanced > Mods, and persisted a checkbox and slider change.

The installed EmblemWeave executable is older and lacks code-mod loading, so the
test used the development build. fefw-recomp 0.2.13 and ObeliskChamber 0.3.2
were inspected but not loaded because their local packages have no prepared
game. No claim is made that the same DLL already works in those two hosts.

## Proposal

### 1. Keep `NxModApi` as the compatibility substrate

Do not bump `NXMOD_API_VERSION` just to advertise NXP. Existing mods should
keep working. The NXP adapter can run on top of v3, v4 and v5 hosts and expose
only the interfaces each host actually has.

This also keeps the first integration small: a host can adopt NXP without
changing its gameplay, hook or file systems.

### 2. Use a separate, versioned NXP ABI for common plugin services

NXP owns:

- plugin metadata and lifecycle;
- host identity and capability discovery;
- settings, files and frame callbacks;
- the common UI canvas;
- future diagnostics and gameplay interfaces.

NXP does not replace each host's game hooks, guest calls, memory access or file
edit system. Those remain host capabilities. A gameplay interface should be a
separate capability, not a requirement for every plugin.

### 3. Align the lookup with issue #165 version 6

Issue #165 proposes one `get_interface(name, version)` member at the end of
`NxModApi`, with interface tables that start with `size` and `version` and grow
only at the end. NXP uses the same pattern at its own host layer.

The two should converge, not compete:

- A legacy mod uses `NxModApi.get_interface`.
- An NXP plugin uses `NxpHostApi.get_interface`.
- A native host can implement both, or use one internally to serve the other.
- Neutral features use `nxp.*`; experimental or host-specific features use the
  project prefix proposed in issue #165.

No host API version should be reserved or changed until the maintainers agree
on the table and the version number.

### 4. Resolve the `open_list` / `add_panel` slot explicitly

The version 5 collision is the clearest example of why a version number is not
a capability probe. A plugin should ask for the feature it needs:

- `nxp.ui` for panels;
- `obeliskchamber.list` (or the agreed neutral name) for the list screen.

Do not require version 5 for panels, and do not call `open_list` through the
`add_panel` slot unless the host explicitly says that is safe.

### 5. Keep `.sites` shared and make unknown kinds non-fatal

The three build scripts currently accept `inline`, `midhook` and `entryhook`
and fail on anything else. Issue #165 proposes skipping unknown kinds with a
warning players can see. That is the right compatibility direction, but it
changes build behavior and should be agreed before implementation.

### 6. Add native host adapters before gameplay

Recommended order:

1. EmblemWeave native NXP adapter, with the same demo DLL loaded through
   `nxp_get_plugin` and no `nxmod_init` dependency.
2. fefw-recomp adapter, exposing only the interfaces its version supports.
3. ObeliskChamber adapter, with the list/panel split made explicit.
4. Launcher manifest and discovery.
5. UI composites and diagnostics.
6. A single gameplay interface, implemented per host.

The first three steps prove the ABI. A gameplay plugin before them would let one
host define the API and force the other two to retrofit.

### 7. Launcher and runtime stay separate

The launcher should read `plugin.json`, validate compatibility, install, enable
or disable, preserve settings and show errors. It should not load plugin DLLs.
The runtime should validate the descriptor and load the code.

A common manifest can serve both, but the launcher must not trust it for
security and the runtime must not trust the plugin's metadata for ABI
compatibility.

## What each project would need to change

### EmblemWeave

- Add a native `NxpHostApi` adapter in the runtime.
- Try `nxp_get_plugin` first, then fall back to `nxmod_init`.
- Expose UI, settings, files, frame and logging with the real host identity.
- Keep API version 4 for now.
- Add loader tests for NXP-only, legacy-only and dual-entry plugins.
- Extend the launcher to read `plugin.json` while keeping `emblemweave.json`
  for existing mods.

### fefw-recomp

- Add the adapter once a source or build environment is available.
- On API 3, expose only log, files, frame and the legacy hook path. Do not fake
  a panel.
- On API 4, expose `nxp.ui`.
- Add manifest discovery to the launcher.
- Keep `.sites` behavior stable until the unknown-kind change is agreed.

### ObeliskChamber

- Add the adapter.
- Split `open_list` and `add_panel` behind explicit capabilities.
- Keep the old `open_list` behavior for existing mods.
- Add manifest discovery.
- Test the same demo DLL on a prepared game.

## What is not proposed

- No silent API version change.
- No forced migration of existing mods.
- No claim that the current NXP UI is the final UI.
- No claim of Linux support from a successful Windows build.
- No binary patching based on speculation.
- No posting this proposal without maintainer review.

## Open questions

1. Is `plugin.json` acceptable as the common manifest, with per-host legacy
   metadata kept alongside it?
2. Should a native host expose NXP interfaces through `NxModApi.get_interface`
   or implement `NxpHostApi` directly?
3. What is the final neutral name for the list-screen interface?
4. Which layer owns gameplay: NXP, `NxModApi`, or host-specific interfaces?
5. What is the support window for `nxmod_init` mods after NXP adoption?
6. Is there a shared signing, hash or registry plan for plugin packages?
7. Should `panel_visible` be part of NXP v1 on every host, or optional?

## Evidence

See `ASSESSMENT.md` for the full comparison, verification ledger and roadmap.
The short version is:

- SDK and demo compile.
- Harness passes.
- EmblemWeave development build loads the demo.
- The native panel renders and persists settings.
- fefw-recomp and ObeliskChamber are inspected, not loaded.
- Gameplay, launcher integration, Linux and unload are not implemented.
