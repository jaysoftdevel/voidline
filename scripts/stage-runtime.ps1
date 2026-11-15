param(
    [Parameter(Mandatory=$true)][string]$BinaryDirectory,
    [Parameter(Mandatory=$true)][string]$Destination,
    [string]$VcpkgInstalled
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$Destination = [IO.Path]::GetFullPath($Destination)
$parent = Split-Path -Parent $Destination
$token = [Guid]::NewGuid().ToString('N')
$stage = Join-Path $parent "runtime-$token"
$previous = Join-Path $parent "runtime-old-$token"
New-Item -ItemType Directory -Force -Path $stage | Out-Null

function Bytes([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) { return 0 }
    $sum = (Get-ChildItem -LiteralPath $Path -File -Recurse | Measure-Object -Property Length -Sum).Sum
    return [long]$sum
}
function Format-Size([string]$Name, [long]$Count) {
    return ('{0,-22} {1,12} bytes ({2:F2} MiB)' -f $Name, $Count, ($Count / 1MB))
}

try {
    Copy-Item -LiteralPath (Join-Path $BinaryDirectory 'voidline.exe') -Destination $stage
    Get-ChildItem -LiteralPath $BinaryDirectory -File -Filter '*.dll' | Copy-Item -Destination $stage
    New-Item -ItemType Directory -Path (Join-Path $stage 'assets') | Out-Null
    Copy-Item -LiteralPath (Join-Path $BinaryDirectory 'assets/game.vpak') -Destination (Join-Path $stage 'assets/game.vpak')
    New-Item -ItemType Directory -Force -Path (Join-Path $stage 'licenses') | Out-Null
    Get-ChildItem -LiteralPath (Join-Path $root 'assets/licenses') -File | Copy-Item -Destination (Join-Path $stage 'licenses')
    $mesa = Join-Path $BinaryDirectory 'mesa'
    if (Test-Path -LiteralPath $mesa -PathType Container) {
        Copy-Item -LiteralPath $mesa -Destination $stage -Recurse
    }
    if ($VcpkgInstalled -and (Test-Path -LiteralPath $VcpkgInstalled -PathType Container)) {
        $licenses = Join-Path $stage 'licenses'
        foreach ($port in (Get-ChildItem -LiteralPath $VcpkgInstalled -Directory)) {
            $notice = Join-Path $port.FullName 'copyright'
            if (Test-Path -LiteralPath $notice -PathType Leaf) {
                Copy-Item -LiteralPath $notice -Destination (Join-Path $licenses ($port.Name + '.txt'))
            }
        }
    }
    @'
@echo off
setlocal
"%~dp0voidline.exe" %*
exit /b %ERRORLEVEL%
'@ | Set-Content -LiteralPath (Join-Path $stage 'run.bat') -Encoding Ascii
    @'
Launch run.bat from this folder. Use run.bat --windowed for a window.
Keep the executable, DLLs, and assets/game.vpak together. Raw image and sound files are not included.
Builds made with Visual Studio use the Microsoft Visual C++ x64 runtime.
To include Direct3D 12 support, add the Mesa runtime with install-mesa.bat in the source folder.
Windows supplies Direct3D and the display driver.
'@ | Set-Content -LiteralPath (Join-Path $stage 'README.txt') -Encoding Ascii

    # Preserve edited JSON files across the atomic runtime-folder replacement.
    foreach ($category in @('levels', 'models')) {
        foreach ($source in @((Join-Path $Destination $category), (Join-Path $BinaryDirectory $category), (Join-Path $root $category))) {
            if (!(Test-Path -LiteralPath $source -PathType Container)) { continue }
            $base = [IO.Path]::GetFullPath($source).TrimEnd('\')
            foreach ($file in (Get-ChildItem -LiteralPath $source -File -Recurse -Filter '*.json')) {
                $relative = $file.FullName.Substring($base.Length).TrimStart('\')
                $target = Join-Path (Join-Path $stage $category) $relative
                if (Test-Path -LiteralPath $target) { continue }
                New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
                Copy-Item -LiteralPath $file.FullName -Destination $target
            }
        }
    }

    & python (Join-Path $PSScriptRoot 'sync-levels.py') (Join-Path $root 'levels') (Join-Path $stage 'levels')
    if ($LASTEXITCODE -ne 0) { throw 'Could not synchronize published levels.' }
    $exe = Join-Path $stage 'voidline.exe'
    $version = & $exe --version
    if ($LASTEXITCODE -ne 0) { throw 'The staged executable could not start.' }
    $dllBytes = [long](Get-ChildItem -LiteralPath $stage -Filter '*.dll' -File | Measure-Object Length -Sum).Sum
    $lines = @(
        $version
        (Format-Size 'Executable' (Bytes $exe))
        (Format-Size 'Assets' (Bytes (Join-Path $stage 'assets')))
        (Format-Size 'Runtime libraries' $dllBytes)
        (Format-Size 'Mesa runtime' (Bytes (Join-Path $stage 'mesa')))
        (Format-Size 'Payload and notices' (Bytes $stage))
        'Payload totals exclude this report and filesystem overhead.'
    )
    $lines | Set-Content -LiteralPath (Join-Path $stage 'BUILD-SIZES.txt') -Encoding Ascii
    if (Test-Path -LiteralPath $Destination) { Move-Item -LiteralPath $Destination -Destination $previous }
    try {
        Move-Item -LiteralPath $stage -Destination $Destination
    } catch {
        if (Test-Path -LiteralPath $previous) { Move-Item -LiteralPath $previous -Destination $Destination }
        throw
    }
    if (Test-Path -LiteralPath $previous) { Remove-Item -LiteralPath $previous -Recurse -Force }
    $lines | Write-Host
    Write-Host "Tester folder: $Destination"
} finally {
    if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
}
