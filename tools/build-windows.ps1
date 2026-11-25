$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$repo = Split-Path -Parent $root
$clang = Join-Path $repo 'EmblemWeave\tools\bin\clang-cl.exe'
$lld = Join-Path $repo 'EmblemWeave\tools\bin\lld-link.exe'
$devShell = 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll'

if (-not (Test-Path $clang)) { throw "clang-cl not found at $clang" }
if (-not (Test-Path $lld)) { throw "lld-link not found at $lld" }
if (-not (Test-Path $devShell)) { throw "Visual Studio DevShell not found at $devShell" }

Import-Module $devShell
Enter-VsDevShell -VsInstallPath 'C:\Program Files\Microsoft Visual Studio\18\Community' `
    -SkipAutomaticLocation -DevCmdArguments '-arch=x64' | Out-Null

& cmake -S $root -B (Join-Path $root 'build') -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_C_COMPILER="$clang" `
    -DCMAKE_CXX_COMPILER="$clang" `
    -DCMAKE_LINKER="$lld"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& cmake --build (Join-Path $root 'build') --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& ctest --test-dir (Join-Path $root 'build') --output-on-failure
exit $LASTEXITCODE
