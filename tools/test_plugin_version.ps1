# SPDX-License-Identifier: MPL-2.0

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$current = & (Join-Path $PSScriptRoot 'get_plugin_version.ps1')
foreach ($file in @('README.md','release/README.txt','release/INSTALL.md','release/RELEASE_NOTES.md')) {
    if ((Get-Content -LiteralPath (Join-Path $repoRoot $file) -Raw) -notmatch "v$([regex]::Escape($current.Version))(?![0-9.])") {
        throw "Current version missing from $file."
    }
}
$root = Join-Path $repoRoot 'build/version-tests'
New-Item -ItemType Directory -Path $root -Force | Out-Null
$fixture = Join-Path $root ([guid]::NewGuid().ToString('N') + '.h')
try {
    foreach ($case in @(@(0,0,0,0),@(0,44,0,44000),@(0,44,1,44001),@(0,44,999,44999),@(0,45,0,45000),@(1,0,0,0))) {
        "#define OPENEMULSION_VERSION_MAJOR $($case[0])`n#define OPENEMULSION_VERSION_MINOR $($case[1])`n#define OPENEMULSION_VERSION_PATCH $($case[2])" |
            Set-Content -LiteralPath $fixture -Encoding ASCII
        $result = & (Join-Path $PSScriptRoot 'get_plugin_version.ps1') -Header $fixture
        if ($result.Version -ne "$($case[0]).$($case[1]).$($case[2])" -or $result.OfxMajor -ne $case[0] -or $result.OfxMinor -ne $case[3]) {
            throw 'Semantic version parsing/OFX encoding failed.'
        }
    }
    foreach ($patch in @('1000','-1','01','not-a-number')) {
        "#define OPENEMULSION_VERSION_MAJOR 0`n#define OPENEMULSION_VERSION_MINOR 44`n#define OPENEMULSION_VERSION_PATCH $patch" |
            Set-Content -LiteralPath $fixture -Encoding ASCII
        $rejected = $false
        try { $null = & (Join-Path $PSScriptRoot 'get_plugin_version.ps1') -Header $fixture } catch { $rejected = $true }
        if (!$rejected) { throw "Invalid patch accepted: $patch" }
    }
} finally { Remove-Item -LiteralPath $fixture -Force }
Write-Host 'Version tracking regression passed.'
