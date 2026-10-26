param([string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/faronaar-chain",[switch]$Baseline,[switch]$RideBaseline,[switch]$AddressSanitizer,[switch]$VerifyNativeCatalog)
$ErrorActionPreference = 'Stop'
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$source=Get-Content "$repo/src/server/scripts/Legion/faronaar_chain.cpp" -Raw
$definition=''
if($Baseline){$definition='/DFARONAAR_COMPANION_BASELINE';$source=(& git -C $repo show '349d886d:src/server/scripts/Legion/faronaar_chain.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned Faronaar baseline missing'}}
if($RideBaseline){if($Baseline){throw 'Choose one pinned baseline'};$definition='/DFARONAAR_RIDE_BASELINE';$source=(& git -C $repo show '31c9f78b:src/server/scripts/Legion/faronaar_chain.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned ride baseline missing'}}
if($AddressSanitizer){$definition+=' /fsanitize=address /Zi'}
[IO.File]::WriteAllText("$output/Faronaar.inc",$source)
foreach ($header in @('ScriptMgr.h','ScriptedCreature.h','Player.h','ObjectAccessor.h','Map.h','TemporarySummon.h','GameObject.h','DatabaseEnv.h','SmartAI.h','SpellScript.h')) {
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
if($VerifyNativeCatalog){
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile FaronaarRideCatalog.cpp -ProbeName FaronaarRideCatalog -OutputFile "$output/ride-native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $rows=@(Get-Content "$output/ride-native.tsv")
 foreach($expected in @("EFFECT`t178923`t0`t28`t0`t180768`t90982`t3635`t46","EFFECT`t180768`t0`t6`t236`t0`t0`t0`t25","EFFECT`t180768`t2`t90`t0`t1`t90982`t0`t1","PROPERTIES`t3635`t4`t8388880","SEAT`t0`t15333`t98`t33554432","SEAT`t1`t15334`t1108377611`t33554432")){if($rows -notcontains $expected){throw "Native Faronaar ride catalog mismatch: $expected"}}
 'PASS matching typed26972/hotfix: native personal vehicle summon/boarding/early-credit and distinct NPC/player seats.'
}
