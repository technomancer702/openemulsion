# SPDX-License-Identifier: MPL-2.0

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$source = Join-Path $repoRoot "dist\OpenEmulsion.ofx.bundle"
$targetRoot = Join-Path $repoRoot "ofx-plugins"
$target = Join-Path $targetRoot "OpenEmulsion.ofx.bundle"

if (Get-Process Resolve -ErrorAction SilentlyContinue) {
    throw "Close DaVinci Resolve before replacing the plugin."
}
$expectedRoot = [IO.Path]::GetFullPath($repoRoot) + [IO.Path]::DirectorySeparatorChar
$target = [IO.Path]::GetFullPath($target)
if (!$target.StartsWith($expectedRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Install target is outside the project: $target"
}

if (!(Test-Path -LiteralPath $source)) {
    throw "Cannot find built bundle at $source. Run .\tools\build_ofx.ps1 first."
}

New-Item -ItemType Directory -Path $targetRoot -Force | Out-Null
if (Test-Path -LiteralPath $target) {
    try {
        Remove-Item -LiteralPath $target -Recurse -Force -ErrorAction Stop
    } catch {
        throw "Cannot replace $target. Close DaVinci Resolve so it releases the loaded OFX file, then rerun .\tools\install_ofx.ps1. Original error: $($_.Exception.Message)"
    }
}
Copy-Item -LiteralPath $source -Destination $targetRoot -Recurse -Force

$current = [Environment]::GetEnvironmentVariable("OFX_PLUGIN_PATH", "User")
$paths = @()
if ($current) {
    $paths = $current -split ';' | Where-Object { $_ -ne "" }
}
if ($paths -notcontains $targetRoot) {
    $paths += $targetRoot
    $updated = ($paths -join ';')
    [Environment]::SetEnvironmentVariable("OFX_PLUGIN_PATH", $updated, "User")
    $env:OFX_PLUGIN_PATH = $updated
    Write-Host "Updated user OFX_PLUGIN_PATH to include $targetRoot"
} else {
    Write-Host "User OFX_PLUGIN_PATH already includes $targetRoot"
}

Write-Host "Installed $target"
Write-Host "Restart Resolve from the Start Menu or a new shell and look for OpenFX > OpenEmulsion > OpenEmulsion."
