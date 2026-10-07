# SPDX-License-Identifier: MPL-2.0

param([switch]$Force)

$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
Push-Location $repoRoot
try {
    $dirty = & git status --porcelain
    if ($LASTEXITCODE -ne 0 -or $dirty) { throw 'Commit source and release files before packaging so the source archive matches the binary.' }
    $revision = (& git rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0) { throw 'Cannot resolve source revision.' }
    $source = Get-Content -LiteralPath 'ofx/src/OpenEmulsion.cpp' -Raw
    $major = [regex]::Match($source, '#define kPluginVersionMajor (\d+)').Groups[1].Value
    $minor = [regex]::Match($source, '#define kPluginVersionMinor (\d+)').Groups[1].Value
    if (!$major -or !$minor) { throw 'Cannot determine plugin version.' }
    $version = "$major.$minor"
    if ((Get-Content -LiteralPath 'release/README.txt' -Raw) -notmatch "OpenEmulsion v$([regex]::Escape($version)) - Windows x64") {
        throw 'Update release instructions and notes for the current plugin version before packaging.'
    }

    & (Join-Path $PSScriptRoot 'build_ofx.ps1')
    & ctest --test-dir build/ofx -C Release --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Regression tests failed; no release will be created.' }

    $cache = Get-Content -LiteralPath 'build/ofx/CMakeCache.txt'
    $compilerLine = $cache | Where-Object { $_ -like 'CMAKE_CXX_COMPILER:FILEPATH=*' } | Select-Object -First 1
    if (!$compilerLine) { throw 'Cannot find the compiler used by CMake.' }
    $compiler = $compilerLine.Substring($compilerLine.IndexOf('=') + 1)
    $compilerVersion = (& $compiler -dumpfullversion).Trim()
    $toolchainRoot = Split-Path (Split-Path $compiler -Parent) -Parent
    $toolchain = Get-Content -LiteralPath (Join-Path $toolchainRoot 'version_info.txt') -Raw
    if ($LASTEXITCODE -ne 0 -or $compilerVersion -ne '16.1.0' -or $toolchain -notmatch 'MinGW-w64 14\.0\.0' -or $toolchain -notmatch 'Thread model: posix') {
        throw 'Runtime notices are prepared for the tested WinLibs GCC 16.1.0 / MinGW-w64 14.0.0 POSIX toolchain. Review them for this compiler.'
    }
    $plugin = Join-Path $repoRoot 'dist/OpenEmulsion.ofx.bundle/Contents/Win64/OpenEmulsion.ofx'
    $objdump = Join-Path (Split-Path $compiler -Parent) 'objdump.exe'
    $imports = & $objdump -p $plugin | Select-String 'DLL Name:' | ForEach-Object { ($_.Line -split 'DLL Name:')[1].Trim() }
    if ($LASTEXITCODE -ne 0 -or !$imports) { throw 'Cannot inspect DLL dependencies.' }
    foreach ($dependency in $imports) {
        if ($dependency -notmatch '^(KERNEL32\.dll|USER32\.dll|comdlg32\.dll|api-ms-win-crt-[a-z0-9-]+\.dll)$') {
            throw "Unexpected DLL dependency $dependency; resolve redistribution requirements before packaging."
        }
    }

    $name = "OpenEmulsion-v$version-Windows-x64"
    $output = Join-Path $repoRoot 'dist/releases'
    New-Item -ItemType Directory -Path $output -Force | Out-Null
    $archive = Join-Path $output "$name.zip"
    $checksum = "$archive.sha256"
    foreach ($file in @($archive,$checksum)) {
        if (Test-Path -LiteralPath $file) {
            if (!$Force) { throw "Already exists: $file. Use -Force to replace this generated release." }
            if (![IO.Path]::GetFullPath($file).StartsWith($output + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) {
                throw 'Release output is outside the expected directory.'
            }
            Remove-Item -LiteralPath $file -Force
        }
    }
    $stagingRoot = Join-Path $repoRoot 'build/release-packaging'
    $temporary = Join-Path $stagingRoot ([guid]::NewGuid().ToString('N'))
    $stage = Join-Path $temporary $name
    New-Item -ItemType Directory -Path $stage -Force | Out-Null
    try {
        Copy-Item -LiteralPath 'dist/OpenEmulsion.ofx.bundle' -Destination $stage -Recurse
        foreach ($file in @('README.txt','INSTALL.md','RELEASE_NOTES.md')) {
            Copy-Item -LiteralPath (Join-Path 'release' $file) -Destination $stage
        }
        Copy-Item -LiteralPath 'LICENSE','THIRD_PARTY_NOTICES.md' -Destination $stage
        Copy-Item -LiteralPath 'docs' -Destination $stage -Recurse
        $sourceFolder = Join-Path $stage 'Source'
        New-Item -ItemType Directory -Path $sourceFolder | Out-Null
        & git archive --format=zip "--prefix=OpenEmulsion-v$version-source/" "--output=$(Join-Path $sourceFolder "OpenEmulsion-v$version-source.zip")" $revision
        if ($LASTEXITCODE -ne 0) { throw 'Cannot create the matching source archive.' }
        $files = @(Get-ChildItem -LiteralPath $stage -File -Recurse | Sort-Object FullName | ForEach-Object {
            [ordered]@{ path=$_.FullName.Substring($stage.Length+1).Replace('\','/'); bytes=$_.Length; sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
        })
        $manifest = [ordered]@{
            product='OpenEmulsion'; version=$version; platform='Windows x64'; status='experimental'; unsigned=$true
            sourceRevision=$revision; sourceUrl="https://github.com/technomancer702/openemulsion/tree/$revision"
            buildToolchain='WinLibs GCC 16.1.0 / MinGW-w64 14.0.0 / UCRT / POSIX threads'
            testedHost='DaVinci Resolve 21.1.1 on Windows x64'; testedGPU='AMD OpenCL gfx1200'
            cleanMachineHostTested=$false; importedSystemDlls=@($imports); files=$files
        }
        $manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $stage 'manifest.json') -Encoding UTF8
        Add-Type -AssemblyName System.IO.Compression.FileSystem
        [IO.Compression.ZipFile]::CreateFromDirectory($stage,$archive,[IO.Compression.CompressionLevel]::Optimal,$true)
        $hash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash
        "$hash  $name.zip" | Set-Content -LiteralPath $checksum -Encoding ASCII
        & (Join-Path $PSScriptRoot 'test_release.ps1') -Archive $archive
        Write-Host "Release ready: $archive"
        Write-Host "SHA-256: $hash"
    } finally {
        $resolved = [IO.Path]::GetFullPath($temporary)
        $expected = [IO.Path]::GetFullPath($stagingRoot) + [IO.Path]::DirectorySeparatorChar
        if (!$resolved.StartsWith($expected,[StringComparison]::OrdinalIgnoreCase)) { throw 'Refusing cleanup outside release staging.' }
        if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse -Force }
    }
} finally { Pop-Location }
