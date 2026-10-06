# SPDX-License-Identifier: MPL-2.0

param([Parameter(Mandatory=$true)][string]$Archive)

$ErrorActionPreference = 'Stop'
if (![Environment]::Is64BitProcess) { throw 'Use 64-bit PowerShell to verify the Windows x64 plugin.' }
$archivePath = [IO.Path]::GetFullPath($Archive)
$repoRoot = [IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$stagingRoot = Join-Path $repoRoot 'build/release-validation'
$temporary = Join-Path $stagingRoot ([guid]::NewGuid().ToString('N'))
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead($archivePath)
try {
    foreach ($entry in $zip.Entries) {
        $path = $entry.FullName.Replace('\','/')
        if ($path -match '(^/|:|(^|/)\.\.(/|$))') { throw "Unsafe archive path: $path" }
    }
} finally { $zip.Dispose() }
try {
    Expand-Archive -LiteralPath $archivePath -DestinationPath $temporary
    $roots = @(Get-ChildItem -LiteralPath $temporary -Directory)
    if ($roots.Count -ne 1 -or @(Get-ChildItem -LiteralPath $temporary -File).Count) { throw 'Release must have one top-level folder.' }
    $root = $roots[0].FullName
    $manifest = Get-Content -LiteralPath (Join-Path $root 'manifest.json') -Raw | ConvertFrom-Json
    if ($manifest.product -ne 'OpenEmulsion' -or $manifest.platform -ne 'Windows x64') { throw 'Unexpected product/platform.' }
    $actualFiles = @(Get-ChildItem -LiteralPath $root -File -Recurse)
    if ($actualFiles.Count -ne $manifest.files.Count + 1) { throw 'Archive contains unexpected or missing files.' }
    $seen = @{}
    foreach ($file in $manifest.files) {
        if ($file.path -match '(^[/\\]|:|(^|[/\\])\.\.([/\\]|$))' -or $seen.ContainsKey($file.path)) { throw 'Invalid or duplicate manifest path.' }
        $seen[$file.path] = $true
        $path = Join-Path $root $file.path
        if (!(Test-Path -LiteralPath $path -PathType Leaf) -or (Get-Item -LiteralPath $path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $path).Hash -ne $file.sha256) {
            throw "Hash/size mismatch: $($file.path)"
        }
    }
    foreach ($required in @('README.txt','INSTALL.md','RELEASE_NOTES.md','LICENSE','THIRD_PARTY_NOTICES.md',
        'OpenEmulsion.ofx.bundle/Contents/Win64/OpenEmulsion.ofx',
        'OpenEmulsion.ofx.bundle/Contents/Resources/org.openemulsion.film.png',
        'OpenEmulsion.ofx.bundle/Contents/Resources/org.openemulsion.film.svg',
        'OpenEmulsion.ofx.bundle/Contents/LICENSE','OpenEmulsion.ofx.bundle/Contents/THIRD_PARTY_NOTICES.md',
        'OpenEmulsion.ofx.bundle/Contents/Licenses/GCC-COPYING3.txt',
        'OpenEmulsion.ofx.bundle/Contents/Licenses/GCC-RUNTIME-EXCEPTION.txt',
        'OpenEmulsion.ofx.bundle/Contents/Licenses/MinGW-w64-RUNTIME.txt',
        'OpenEmulsion.ofx.bundle/Contents/Licenses/Winpthreads-COPYING.txt',
        "Source/OpenEmulsion-v$($manifest.version)-source.zip")) {
        if (!$seen.ContainsKey($required)) { throw "Missing release requirement: $required" }
    }
    if (!('OpenEmulsionReleaseProbe' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class OpenEmulsionReleaseProbe {
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    public static extern IntPtr LoadLibraryEx(string path, IntPtr file, uint flags);
    [DllImport("kernel32.dll", CharSet=CharSet.Ansi)]
    public static extern IntPtr GetProcAddress(IntPtr module, string name);
    [DllImport("kernel32.dll")] public static extern bool FreeLibrary(IntPtr module);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate int Count();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate IntPtr GetPlugin(int index);
    [StructLayout(LayoutKind.Sequential)] public struct Plugin {
        public IntPtr api; public int apiVersion; public IntPtr identifier;
        public uint major; public uint minor; public IntPtr setHost; public IntPtr mainEntry;
    }
    public static string Inspect(string path) {
        // Search the plugin directory and Windows system directories, not compiler/PATH DLLs.
        IntPtr module = LoadLibraryEx(path, IntPtr.Zero, 0x00000100 | 0x00000800);
        if (module == IntPtr.Zero) throw new Exception("Cannot load extracted plugin; Windows error " + Marshal.GetLastWin32Error());
        try {
            IntPtr countPtr = GetProcAddress(module, "OfxGetNumberOfPlugins");
            IntPtr pluginPtr = GetProcAddress(module, "OfxGetPlugin");
            if (countPtr == IntPtr.Zero || pluginPtr == IntPtr.Zero) throw new Exception("Missing OFX exports");
            var count = (Count)Marshal.GetDelegateForFunctionPointer(countPtr, typeof(Count));
            var get = (GetPlugin)Marshal.GetDelegateForFunctionPointer(pluginPtr, typeof(GetPlugin));
            if (count() != 1) throw new Exception("Unexpected OFX effect count");
            var plugin = (Plugin)Marshal.PtrToStructure(get(0), typeof(Plugin));
            if (Marshal.PtrToStringAnsi(plugin.api) != "OfxImageEffectPluginAPI" || plugin.apiVersion != 1 ||
                Marshal.PtrToStringAnsi(plugin.identifier) != "org.openemulsion.film" ||
                plugin.setHost == IntPtr.Zero || plugin.mainEntry == IntPtr.Zero) throw new Exception("Invalid OFX plugin metadata");
            return plugin.major + "." + plugin.minor;
        } finally { FreeLibrary(module); }
    }
}
'@
    }
    $version = [OpenEmulsionReleaseProbe]::Inspect((Join-Path $root 'OpenEmulsion.ofx.bundle/Contents/Win64/OpenEmulsion.ofx'))
    if ($version -ne $manifest.version) { throw 'Binary version differs from the release manifest.' }
    $checksumPath = "$archivePath.sha256"
    if (!(Test-Path -LiteralPath $checksumPath) -or ((Get-Content -LiteralPath $checksumPath -Raw) -split '\s+')[0] -ne (Get-FileHash -LiteralPath $archivePath).Hash) {
        throw 'ZIP checksum is missing or incorrect.'
    }
    Write-Host "Verified v${version}: $($manifest.files.Count) payload files, all hashes/sizes, required licenses/icon/source, ZIP checksum, and native OFX load/exports without compiler DLL search."
} finally {
    $resolved = [IO.Path]::GetFullPath($temporary)
    $expected = [IO.Path]::GetFullPath($stagingRoot) + [IO.Path]::DirectorySeparatorChar
    if (!$resolved.StartsWith($expected,[StringComparison]::OrdinalIgnoreCase)) { throw 'Refusing cleanup outside release validation.' }
    if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse -Force }
}
