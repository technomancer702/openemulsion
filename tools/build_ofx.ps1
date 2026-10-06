# SPDX-License-Identifier: MPL-2.0

param(
    [string]$ResolveOpenFXRoot,
    [string]$Generator = "Ninja"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot "build\ofx"
$cmakeArgs = @("-S", (Join-Path $repoRoot "ofx"), "-B", $buildDir,
    "-G", $Generator, "-DCMAKE_BUILD_TYPE=Release")
if ($ResolveOpenFXRoot) {
    $cmakeArgs += "-DRESOLVE_OPENFX_ROOT=$ResolveOpenFXRoot"
}
& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed." }
& cmake --build $buildDir --config Release
if ($LASTEXITCODE -ne 0) { throw "Plugin build failed." }

Write-Host "Built bundle:"
Write-Host (Join-Path $repoRoot "dist\OpenEmulsion.ofx.bundle")
