param(
    [string]$FefwRoot = "",
    [string]$ObeliskRoot = "",
    [string]$TrinityRoot = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = "Stop"

function Resolve-Checkout([string]$explicit, [string]$name, [string]$parameter) {
    if ($explicit) { return (Resolve-Path $explicit).Path }
    $candidates = @(
        (Join-Path (Split-Path -Parent $TrinityRoot) $name),
        (Join-Path (Split-Path -Parent (Split-Path -Parent $TrinityRoot)) $name)
    )
    foreach ($candidate in $candidates) {
        if (Test-Path (Join-Path $candidate "launcher")) { return (Resolve-Path $candidate).Path }
    }
    throw "Could not find $name. Pass -$parameter with the checkout path."
}

$sharedFiles = @(
    "shared/catalog.json",
    "shared/prefixes.json",
    "shared/prefix-core.mjs",
    "ui/trinity-ui.js",
    "ui/trinity.css"
)

$targets = @(
    @{ Root = (Resolve-Checkout $FefwRoot "fefw-recomp" "FefwRoot"); Adapter = "hosts/launchers/fefw/trinity.js" },
    @{ Root = (Resolve-Checkout $ObeliskRoot "ObeliskChamber" "ObeliskRoot"); Adapter = "hosts/launchers/obelisk/trinity.js" }
)

foreach ($target in $targets) {
    $destination = Join-Path $target.Root "launcher/src/trinity"
    New-Item -ItemType Directory -Force -Path $destination | Out-Null
    foreach ($relative in $sharedFiles) {
        $name = Split-Path -Leaf $relative
        Copy-Item (Join-Path $TrinityRoot $relative) (Join-Path $destination $name) -Force
        Write-Host "synced $name -> $destination"
    }
    Copy-Item (Join-Path $TrinityRoot $target.Adapter) (Join-Path $target.Root "launcher/src/pages/trinity.js") -Force
    Write-Host "synced trinity.js -> $($target.Root)/launcher/src/pages"
}
