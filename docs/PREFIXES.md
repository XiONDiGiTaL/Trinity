# Trinity prefix specification

Standard id: `trinity-prefix/1`

Status: implemented in the catalog and the reference plugin; proposed for host adoption.

## 1. Problem

Three hosts use different API versions and different table layouts. A shared plugin needs to ask for a feature without guessing from a version number. A launcher or runtime also needs to know which project owns an interface name.

The prefix registry makes ownership explicit.

## 2. Canonical form

An interface name is a dotted, lowercase namespace:

```text
<owner>.<local-name>
```

Rules:

- Segments use `[a-z0-9-]` and start with a letter or digit.
- At least one owner segment and one local segment are required.
- No leading or trailing dot, no empty segment, no uppercase.
- The first segment is the owner.
- The full name is the identity.

Examples:

```text
trinity.core.panel
nxp.ui
emblemweave.ui.panel
fefw-recomp.ui.panel
obeliskchamber.ui.panel
```

## 3. Registered namespaces

| Owner | Canonical prefix | Aliases | API at inspection | Meaning |
| --- | --- | --- | --- | --- |
| Trinity | `trinity.` | none | N/A | Portable services owned by the shared layer |
| NXP | `nxp.` | none | ABI v1 | The compatibility ABI |
| EmblemWeave | `emblemweave.` | `ew.` | `nxmod-api-4` | Host-only interfaces and game integrations |
| fefw-recomp | `fefw-recomp.` | `fefw.` | `nxmod-api-3` | Host-only interfaces and game integrations |
| ObeliskChamber | `obeliskchamber.` | `oc.` | `nxmod-api-5` | Host-only interfaces and game integrations |

Canonical names are written to disk. Aliases are accepted for input and diagnostics and reported alongside the canonical prefix. An alias must not become a second on-disk identity.

Reserved first segments:

```text
core, hub, ui, settings, files, frame, diagnostics, compat, prefixes
```

A plugin cannot claim a reserved first segment as a host namespace.

## 4. Resolution algorithm

1. Reject non-strings, empty strings, uppercase, trailing dots, empty segments and invalid characters.
2. Match the canonical prefixes and aliases.
3. If a neutral prefix matches, classify as `portable` (`trinity.`) or `abi` (`nxp.`).
4. If a host prefix or alias matches, classify as `host` and report both the canonical prefix and the alias used.
5. If a syntactically valid name has no registered owner, classify as `unknown`.
6. Never silently accept an unknown owner.

A host answers for its own prefix and no other host's prefix. A shared plugin must use `trinity.` for portable services, not a host name.

## 5. Relationship to fefw-recomp issue #165

Issue #165 proposes:

- one `get_interface(name, version)` member appended to `NxModApi`;
- interface tables that start with `size` and `version` and grow only at the end;
- experimental names prefixed with the owning project;
- `open_list` moved out of the version-gated table slot and behind a named interface.

Trinity adopts the naming and lookup shape. It does not claim that issue #165 is accepted or implemented.

There are two lookups during the transition:

- A legacy mod calls `NxModApi.get_interface` when the host provides version 6.
- An NXP plugin calls `NxpHostApi.get_interface`, which exists in NXP v1.

A native host can implement both. The compatibility adapter currently uses the legacy `NxModApi` table directly, so it cannot discover an interface that exists only in a future version 6 table. That is a known convergence task, not a hidden claim.

## 6. Naming rules for host features

- `nxp.ui` is the portable panel service. Do not call it `emblemweave.ui` just because EmblemWeave implements it first.
- `obeliskchamber.list` is a host-specific list screen. It is not `nxp.ui` unless all three hosts agree on a common list contract.
- `fefw-recomp.sites` would be a host-specific build or site service. It is not `trinity.sites` unless it is portable.
- A neutral service that depends on one host's implementation still belongs to that host until the other hosts implement it.
- A service that is genuinely portable gets a `trinity.` or `nxp.` name and a version.

## 7. Implementation

| Artifact | Role |
| --- | --- |
| `shared/prefixes.json` | Canonical registry |
| `shared/prefix-core.mjs` | JavaScript parser and classifier |
| `shared/prefix-core.test.mjs` | Node tests |
| `plugins/trinity_core/trinity_plugin.cpp` | C++ classifier and runtime check |
| `ui/trinity-ui.js` | Launcher registry card and interactive checker |
| `tools/validate_plugin.py` | Manifest capability name validation |

The C++ and JavaScript classifiers intentionally share rules, not code. A future native host should use a generated table or a small shared C library if drift becomes a risk.

## 8. Compatibility

- Existing `NxModApi` symbols remain valid.
- No host API version is changed by this specification.
- A plugin that uses an unknown prefix is rejected by diagnostic tools but still loads if the host does not enforce the guard. Enforcement must be explicit, not accidental.
- The reference plugin logs accepted and rejected sample names at initialization, so a runtime log shows the classifier is active.

## 9. Open decisions

1. Whether aliases should be accepted by the runtime or only by launchers and diagnostics.
2. Whether the reserved list is global or per host.
3. The final neutral name for the ObeliskChamber list interface.
4. Whether a future `NxModApi` version 6 should expose the same registry or generate it from `shared/prefixes.json`.
5. Whether duplicate ownership should fail a build or only a package validation.
