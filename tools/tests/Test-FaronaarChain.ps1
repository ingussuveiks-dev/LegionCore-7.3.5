param([string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/faronaar-chain",[switch]$Baseline,[switch]$AddressSanitizer)
$ErrorActionPreference = 'Stop'
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$source=Get-Content "$repo/src/server/scripts/Legion/faronaar_chain.cpp" -Raw
$definition=''
if($Baseline){$definition='/DFARONAAR_COMPANION_BASELINE';$source=(& git -C $repo show '349d886d:src/server/scripts/Legion/faronaar_chain.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned Faronaar baseline missing'}}
if($AddressSanitizer){$definition+=' /fsanitize=address /Zi'}
[IO.File]::WriteAllText("$output/Faronaar.inc",$source)
foreach ($header in @('ScriptMgr.h','ScriptedCreature.h','Player.h','ObjectAccessor.h','Map.h','TemporarySummon.h','GameObject.h','DatabaseEnv.h')) {
    [IO.File]::WriteAllText("$output/$header", '#pragma once')
}
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$commands = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 $definition /I"$output" "$PSScriptRoot/FaronaarChainTest.cpp" /Fe"$output/FaronaarChainTest.exe" /Fo"$output/FaronaarChainTest.obj" /Fd"$output/FaronaarChainTest.pdb"
if errorlevel 1 exit /b 1
"$output/FaronaarChainTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd", $commands)
& "$output/run.cmd"
if ($LASTEXITCODE) { throw 'Faronaar chain regression failed' }
