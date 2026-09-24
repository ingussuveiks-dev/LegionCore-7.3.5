param([string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/pet-load-selection")
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source = [IO.File]::ReadAllText("$repo/src/server/game/Entities/Pet/Pet.cpp")
$start = $source.IndexOf('std::pair<PetStable::PetInfo const*, PetSaveMode> Pet::GetLoadPetInfo(')
$end = $source.IndexOf('class PetLoadQueryHolder', $start)
if ($start -lt 0 -or $end -le $start) { throw 'Cannot locate production pet selector' }
[IO.File]::WriteAllText("$output/PetLoadSelection.inc", $source.Substring($start, $end - $start))
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$commands = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$repo/src/common" /I"$repo/src/common/Utilities" /I"$repo/src/server/game/Entities/Pet" /I"$output" "$PSScriptRoot/PetLoadSelectionTest.cpp" /Fe"$output/PetLoadSelectionTest.exe" /Fo"$output/PetLoadSelectionTest.obj"
if errorlevel 1 exit /b 1
"$output/PetLoadSelectionTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd", $commands)
& "$output/run.cmd"
if ($LASTEXITCODE -ne 0) { throw "Pet lookup regression failed: $LASTEXITCODE" }
Write-Output 'Pet lookup regression passed.'
