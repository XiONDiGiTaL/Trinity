# Building and testing Trinity

These instructions are for contributors to Trinity and for host developers who want to build the plugin from source. Launchers and end users should consume the prebuilt [release artifacts](INSTALL.md) instead.

The public ABI is [include/nxp/nxp.h](../include/nxp/nxp.h). A native host needs only that header; the compatibility adapter additionally compiles against public host headers from an EmblemWeave checkout.

## Windows x64

Requirements:

- CMake 3.20 or newer
- Ninja
- Visual Studio 2022 or newer
- The `clang-cl` and `lld-link` binaries used by the development workspace (normally under `EmblemWeave/tools/bin`)

From the workspace root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Trinity/tools/build-windows.ps1
```

The script imports the Visual Studio developer shell, configures a Release build with Ninja, builds it and runs CTest. Outputs:

```text
Trinity/build/Trinity.dll
Trinity/build/NxpDemo.dll
Trinity/build/nxp_host_test.exe
Trinity/build/nxmod_adapter_test.exe
```

The compatibility adapter compiles against `runtime/include/nxrt/mod_api.h` and `recomp/include/nxrecomp/ctx.h` from an EmblemWeave checkout. Pass `-DNXP_NXMOD_ROOT=<checkout>/engine/fefw-recomp` when the checkout is elsewhere. `tools/build-windows.ps1` assumes the default sibling layout.

## Linux x64

Ninja, CMake and a C++17 compiler are enough. The adapter uses `/proc/self/exe` for host detection; the host harness uses `dlopen`.

```bash
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DNXP_NXMOD_ROOT=../EmblemWeave/engine/fefw-recomp
cmake --build build-linux
ctest --test-dir build-linux --output-on-failure
./build-linux/nxp_host_test ./build-linux/libTrinity.so trinity
```

Outputs:

```text
build-linux/libTrinity.so
build-linux/libNxpDemo.so
build-linux/nxp_host_test
build-linux/nxmod_adapter_test
```

The Linux output is an ELF64 x86-64 shared object. `readelf -h`, `nm -D` and `ldd` are part of the verification; a renamed Windows DLL is not a Linux build. No Linux game launcher or game runtime exists in this workspace, so the Linux claim stops at native loading.

## Tests

```powershell
ctest --test-dir Trinity/build --output-on-failure
node --test Trinity/shared/prefix-core.test.mjs
python Trinity/tools/validate_plugin.py Trinity/plugins/trinity_core/plugin.json --allow-missing-entry
python -m unittest discover -s Trinity/tests -p "test_*.py"
```

The source tree has `plugin.json` but not the built DLLs beside it, so the validator runs with `--allow-missing-entry` for the metadata check. Run it without that flag against an extracted release to verify the entry files are present.

The C++ host harness loads the plugin, supplies an NXP host with all four interfaces, checks discovery and panel registration, exercises settings and reset, runs the frame callback, calls shutdown and checks the final flush. Three more CTest cases run the compatibility adapter against simulated `NxModApi` 3, 4 and 5 tables. The same five tests pass on Windows and Linux.

## Bounded fefw runs

`tools/check_fefw_deadline.py` runs `fefw.exe` directly and treats the runtime's own exit status `124` at `--max-seconds` as the expected deadline exit. It reports an overrun only when the process is still running at the configured limit and must be killed:

```powershell
python Trinity/tools/check_fefw_deadline.py --exe X:\fefw-recomp-0.2.13\fefw.exe `
  --max-seconds 25 --ini X:\fefw-recomp-0.2.13\fefw.ini --headless
```

The default grace window is 15 seconds past the deadline, matching the runtime's bounded teardown. A shell wrapper that checks only `$?` cannot distinguish the deadline exit from GNU `timeout`'s own `124`.

## What the tests do not prove

The harnesses exercise the ABI, the adapter and the reference plugin. They do not prove a game runtime, a rendered panel in every host, or gameplay. Record runtime results in [COMPATIBILITY.md](COMPATIBILITY.md) at the level actually observed.
