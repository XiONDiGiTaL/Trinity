# EmblemWeave launcher adapter

EmblemWeave's launcher is native egui/Rust. It cannot import the HTML/CSS surface used by the two Tauri launchers.

The integration contract is:

1. Add one `Page::Trinity` variant and one `NavGroup::Trinity` group in `apps/emblemweave/src/pages/mod.rs`.
2. Add the page to `Page::ALL`, give it a label and an icon, and add the group to `NavGroup::ALL`.
3. Keep the navigation invariant: every page belongs to exactly one group. The existing tests enforce this.
4. Read `shared/catalog.json` and `shared/prefixes.json` at compile time with `include_str!` and `serde_json`.
5. Draw host cards, the listing table and the prefix table with the existing `widgets` and palette. Do not import a second design system.
6. Add the native NXP adapter in the runtime separately. The launcher page must not load plugin DLLs.

No EmblemWeave source file has been changed by this repository. The page is a host integration task, not a shared UI file. The exact egui implementation belongs in the EmblemWeave checkout and its review process.

The shared data files are stable inputs. A launcher can embed them, fetch them at build time, or copy them through a release script; it must not edit them at runtime.
