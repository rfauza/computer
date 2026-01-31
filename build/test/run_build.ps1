<#
Runs the build: change to the `build` directory, run `make clean` then `make all`.
Place this script in the project root and run from PowerShell.
#>
Set-StrictMode -Version Latest

$buildDir = Join-Path -Path $PSScriptRoot -ChildPath 'build'
if (-not (Test-Path $buildDir)) {
    Write-Error "Build directory '$buildDir' not found."
    exit 1
}

Push-Location $buildDir
try {
    Write-Host "Running: make clean"
    & make clean

    Write-Host "Running: make all"
    & make all
} finally {
    Pop-Location
}

Write-Host "Build script finished."
