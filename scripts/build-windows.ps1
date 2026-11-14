param(
    [ValidateSet('Release','Debug')]
    [string]$Configuration = 'Release',
    [string]$VcpkgRoot = $env:VCPKG_ROOT,
    [string]$Generator = 'Visual Studio 17 2022',
    [switch]$NoTests
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root 'build-windows'
if (-not $VcpkgRoot) {
    $VcpkgRoot = Join-Path $env:LOCALAPPDATA 'vcpkg'
}
$VcpkgRoot = [IO.Path]::GetFullPath($VcpkgRoot)
$toolchain = Join-Path $VcpkgRoot 'scripts\buildsystems\vcpkg.cmake'
if (-not (Test-Path -LiteralPath $toolchain -PathType Leaf)) {
    throw 'Set VCPKG_ROOT to a bootstrapped vcpkg checkout. See docs\WINDOWS.md.'
}
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw 'CMake is required. Install it or use the Visual Studio Developer Command Prompt.'
}
function Invoke-Checked {
    param([string]$Program, [string[]]$Arguments)
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE." }
}
$testFlag = if ($NoTests) { 'OFF' } else { 'ON' }
$configure = @(
    '-S', $root, '-B', $build, '-G', $Generator, '-A', 'x64',
    "-DCMAKE_TOOLCHAIN_FILE=$toolchain",
    '-DVCPKG_TARGET_TRIPLET=x64-windows', '-DVCPKG_APPLOCAL_DEPS=ON',
    "-DBUILD_TESTING=$testFlag"
)
Invoke-Checked cmake $configure
Invoke-Checked cmake @('--build', $build, '--config', $Configuration, '--parallel')
if (-not $NoTests) {
    Invoke-Checked ctest @('--test-dir', $build, '-C', $Configuration, '--output-on-failure')
}
$exe = Join-Path $build "voidline\$Configuration\voidline.exe"
Invoke-Checked $exe @('--version')
Write-Host "Executable: $exe"
