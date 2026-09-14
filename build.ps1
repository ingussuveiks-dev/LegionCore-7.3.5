$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmakeCommand) {
    $cmake = $cmakeCommand.Source
} else {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $vs = & $vswhere -latest -products '*' -property installationPath
    $cmake = Join-Path $vs 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
}
if (!(Test-Path $cmake)) { throw 'Install Visual Studio 2022 with C++ and CMake support.' }
& $cmake --preset default
if ($LASTEXITCODE) { exit $LASTEXITCODE }
& $cmake --build --preset default
if ($LASTEXITCODE) { exit $LASTEXITCODE }
& $cmake --install build-extractors --config Release
exit $LASTEXITCODE
