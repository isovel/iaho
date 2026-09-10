#Requires -Version 5.1
<#
.SYNOPSIS
    Configures and builds the I Already Have One UE4SS mod, then installs it into the game.

.DESCRIPTION
    Run from a Developer PowerShell for VS 2022, or with cmake/ninja on PATH and
    a v143 (>= 14.43) MSVC toolset available.

.PARAMETER GameDir
    Palworld install root. Defaults to the usual Steam location.

.PARAMETER Install
    Copy the built dll plus config.ini and enabled.txt into the game's ue4ss\Mods directory.
#>
param(
    [string]$GameDir = 'S:\SteamLibrary\steamapps\common\Palworld',
    [switch]$Install
)

$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent $PSScriptRoot
$ModsDir = Join-Path $GameDir 'Pal\Binaries\Win64\ue4ss\Mods'
$BuildDir = Join-Path $RepoRoot 'build'

Push-Location $RepoRoot
try {
    cmake -B $BuildDir -G Ninja -DCMAKE_BUILD_TYPE=Game__Shipping__Win64
    if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }

    cmake --build $BuildDir --target IAlreadyHaveOne
    if ($LASTEXITCODE -ne 0) { throw "build failed" }

    if ($Install) {
        $Target = Join-Path $ModsDir 'IAlreadyHaveOne'
        New-Item -ItemType Directory -Force -Path (Join-Path $Target 'dlls') | Out-Null

        $Dll = Get-ChildItem -Path $BuildDir -Recurse -Filter 'IAlreadyHaveOne.dll' | Select-Object -First 1
        if (-not $Dll) { throw "IAlreadyHaveOne.dll not found under $BuildDir" }

        Copy-Item $Dll.FullName (Join-Path $Target 'dlls\main.dll') -Force
        # config.ini is deliberately not overwritten once it exists, so local edits survive a rebuild.
        if (-not (Test-Path (Join-Path $Target 'config.ini'))) {
            Copy-Item (Join-Path $RepoRoot 'mod\payload\config.ini') (Join-Path $Target 'config.ini')
        }
        Copy-Item (Join-Path $RepoRoot 'mod\payload\enabled.txt') (Join-Path $Target 'enabled.txt') -Force

        Write-Host "Installed to $Target"
    }
}
finally {
    Pop-Location
}
