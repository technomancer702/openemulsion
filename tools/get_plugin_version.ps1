# SPDX-License-Identifier: MPL-2.0

param([string]$Header = (Join-Path $PSScriptRoot '../ofx/src/PluginVersion.h'))

$ErrorActionPreference = 'Stop'
$source = Get-Content -LiteralPath $Header -Raw
$parts = @{}
foreach ($part in @('MAJOR','MINOR','PATCH')) {
    $match = [regex]::Match($source, "(?m)^#define OPENEMULSION_VERSION_$part (0|[1-9][0-9]*)\s*$")
    if (!$match.Success) { throw "Cannot determine plugin version $part from $Header." }
    $parts[$part] = [uint32]$match.Groups[1].Value
}
if ($parts.PATCH -ge 1000 -or $parts.MINOR -gt 4294966) { throw 'Version exceeds the OFX encoding range.' }
[pscustomobject]@{
    Version = "$($parts.MAJOR).$($parts.MINOR).$($parts.PATCH)"
    OfxMajor = $parts.MAJOR
    OfxMinor = [uint32]($parts.MINOR * 1000 + $parts.PATCH)
}
