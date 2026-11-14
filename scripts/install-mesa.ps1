param(
    [Parameter(Mandatory=$true,Position=0)]
    [string]$Source,
    [ValidateSet('Release','Debug')]
    [string]$Configuration = 'Release'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$bin = Join-Path $root "build-windows\voidline\$Configuration"
if (-not (Test-Path -LiteralPath (Join-Path $bin 'voidline.exe') -PathType Leaf)) {
    throw 'Build VOIDLINE before adding its Mesa compatibility runtime.'
}
$sourceDir = (Resolve-Path -LiteralPath $Source).Path
$x64 = Join-Path $sourceDir 'x64'
if (Test-Path -LiteralPath $x64 -PathType Container) { $sourceDir = $x64 }

function Assert-X64Pe([string]$Path) {
    $file = [IO.File]::OpenRead($Path)
    try {
        $reader = [IO.BinaryReader]::new($file)
        if ($file.Length -lt 64 -or $reader.ReadUInt16() -ne 0x5a4d) { throw "Invalid DLL: $Path" }
        $file.Position = 60
        $header = $reader.ReadUInt32()
        if ($header -gt ($file.Length - 6)) { throw "Invalid DLL: $Path" }
        $file.Position = $header
        if ($reader.ReadUInt32() -ne 0x4550 -or $reader.ReadUInt16() -ne 0x8664) {
            throw "An x64 DLL is required: $Path"
        }
    } finally {
        $file.Dispose()
    }
}

$required = @('opengl32.dll','dxil.dll')
$optional = @('libgallium_wgl.dll','opengl32sw.dll','libglapi.dll')
$files = @()
foreach ($name in $required + $optional) {
    $path = Join-Path $sourceDir $name
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        if ($required -contains $name) { throw "The extracted Mesa x64 directory is missing $name." }
        continue
    }
    Assert-X64Pe $path
    $files += $path
}

$destination = Join-Path $bin 'mesa'
$staging = Join-Path $bin ("mesa.new." + [Guid]::NewGuid().ToString('N'))
$previous = Join-Path $bin ("mesa.old." + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $staging | Out-Null
try {
    foreach ($file in $files) { Copy-Item -LiteralPath $file -Destination $staging }
    if (Test-Path -LiteralPath $destination) { Move-Item -LiteralPath $destination -Destination $previous }
    try {
        Move-Item -LiteralPath $staging -Destination $destination
    } catch {
        if (Test-Path -LiteralPath $previous) { Move-Item -LiteralPath $previous -Destination $destination }
        throw
    }
    if (Test-Path -LiteralPath $previous) { Remove-Item -LiteralPath $previous -Recurse -Force }
} finally {
    if (Test-Path -LiteralPath $staging) { Remove-Item -LiteralPath $staging -Recurse -Force }
}
Write-Host "Installed the game-local Mesa runtime in $destination"
$build = Join-Path $root 'build-windows'
$dist = if ($Configuration -eq 'Release') { 'dist\VOIDLINE' } else { 'dist\VOIDLINE-Debug' }
& (Join-Path $PSScriptRoot 'stage-runtime.ps1') -BinaryDirectory $bin `
    -Destination (Join-Path $build $dist) -VcpkgInstalled (Join-Path $build 'vcpkg_installed\x64-windows\share')
