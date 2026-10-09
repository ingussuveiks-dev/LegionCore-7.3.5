param([string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/azsuna-scythe")
$ErrorActionPreference = 'Stop'
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
foreach ($header in @('ScriptMgr.h','ScriptedCreature.h','Player.h','ObjectAccessor.h','Map.h','TemporarySummon.h','GameObject.h')) {
    [IO.File]::WriteAllText("$output/$header", '#pragma once')
}
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$commands = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/AzsunaScytheTest.cpp" /Fe"$output/AzsunaScytheTest.exe" /Fo"$output/AzsunaScytheTest.obj"
if errorlevel 1 exit /b 1
"$output/AzsunaScytheTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd", $commands)
& "$output/run.cmd"
if ($LASTEXITCODE) { throw 'Azsuna scythe regression failed' }
