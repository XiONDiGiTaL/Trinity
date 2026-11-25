# Licensing and provenance

## Repository license

The repository's top-level `LICENSE` is the MIT license established by the repository owner. It covers original Trinity documentation, the catalog, the launcher surface and the brand assets unless a file says otherwise.

## GPL-3.0-or-later material

The following files keep the `GPL-3.0-or-later` SPDX identifier they carried before migration:

- `include/nxp/nxp.h`
- `hosts/compat/nxmod_adapter.cpp`
- `plugins/trinity_core/trinity_plugin.cpp`
- `examples/hello_panel/hello_panel.cpp`
- `tests/nxp_host_test.cpp`

The compatibility adapter and the ABI are derived from the public `NxModApi` interface shared by EmblemWeave, fefw-recomp and ObeliskChamber. Those projects are GPL-3.0-or-later. The GPL files are distributed as part of this repository with their notices intact. A downstream distributor must comply with the GPL for those files and with the MIT license for the rest.

This is a source-level licensing record, not legal advice. A maintainer should review the combined distribution before a public binary release.

## Host code

No host source tree is vendored. The compatibility adapter compiles against public headers from an EmblemWeave checkout supplied by the developer. Those headers are not copied into this repository.

The launcher surface was informed by the public Tauri launcher source of fefw-recomp and ObeliskChamber. It does not copy their page code or assets. The host adapters are small modules that a host project can review and adopt on its own terms.

## Brand

The Trinity mark and lockup are vector redraws derived from a logo supplied by the project owner. The original raster is not committed. The fonts referenced by the SVGs are system fonts and are not distributed.

## Evidence

`docs/evidence/` contains screenshots and a runtime log from a real EmblemWeave development build. They are test evidence, not code. They may show the host's UI and the demo panel. Do not reuse them as marketing material without reviewing what the host window contains.

## FCC Desktop reference

The launcher surface uses general layout ideas from the FCC Desktop model library: grouped cards, a search field, status rows and a compact footer. No FCC code, stylesheet or asset is copied. The design tokens in Trinity are its own.

## Dependencies

The C++ build uses CMake, Ninja and the compiler toolchain already present in the development environment. It does not vendor a JSON library, an ImGui copy or a host SDK. The Python validator and tests use only the standard library. The launcher UI uses no runtime dependency beyond the host's existing Vite bundle.
