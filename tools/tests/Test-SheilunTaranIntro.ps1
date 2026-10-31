param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyNativeCatalog,[switch]$VerifyReleaseBinary,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
$path='src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/Sheylun.cpp';$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$old=(& git -C $repo show "3ba6d8c6f:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Taran source missing'}
$normal=$source
foreach($line in @('#include "ObjectAccessor.h"','#include "ScenarioMgr.h"','#include "Scenario.h"','        #include "sheilun_taran_intro.h"','            CancelIntroHeal();')){$normal=$normal.Replace($line,'')}
$normal=$normal.Replace((Block $normal 'void HealReceived('),(Block $old 'void HealReceived('))
if(($normal -replace '\s','') -cne ($old -replace '\s','')){throw 'Sheilun changed beyond Taran applied opening heal, reset cancellation and helper/include declarations'}
& "$PSScriptRoot/Test-CreatureHealApplied.ps1" -ScopeOnly;if(!$?){throw 'Actual post-heal dispatch source differs'}
if($ScopeOnly){'PASS whole Sheilun pinned3ba6d8c6f outside applied opening heal; combat wound/damage, all routes/gossip/boss/pickup/registration unchanged';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sheilun-taran-intro$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$selected=if($Baseline){$old}else{$source}
[IO.File]::WriteAllText("$out/HealReceived.inc",(Block $selected 'void HealReceived('))
[IO.File]::WriteAllText("$out/Reset.inc",(Block $selected 'void Reset('))
Copy-Item -LiteralPath "$repo/src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/sheilun_taran_intro.h" -Destination "$out/Intro.inc"
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw;$heal=Block $unit 'int32 Unit::DealHeal(';$last=$heal.IndexOf('    Unit* unit = this;');if($last -lt 0){throw 'Native heal boundary changed'}
[IO.File]::WriteAllText("$out/DealHeal.inc",$heal.Substring(0,$last).Replace(', SpellInfo const* spellProto','')+"    return gain;`n}")
$creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/Notify.inc",(Block $creature 'void Creature::NotifyHealApplied('))
$update=Block $creature 'void Creature::Update(uint32 diff)';$last=$update.IndexOf('    volatile uint32 creatureEntry');if($last -lt 0){throw 'Native lifecycle boundary changed'}
[IO.File]::WriteAllText("$out/Tick.inc",$update.Substring(0,$last)+"    m_isUpdate = false;`n}")
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath;$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SheilunTaranIntroTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Taran applied opening-heal regression failed'}
if($VerifyNativeCatalog){
 $prior=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SheilunCatalog.cpp -ProbeName SheilunCatalog -OutputFile "$out/native.tsv";if(!$?){throw 'Native Sheilun catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
 $rows=Get-Content "$out/native.tsv"
 if($rows -notcontains "CRITERION`t0`t46576`t29397`t92`t48371" -or $rows[-1] -cne "EOF`tsheilun"){throw 'Native opening48371/46576 contract differs'}
}
if($VerifyReleaseBinary){
 $prior=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$prior";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sheilun_taran_intro_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Taran opening heal missing'}}finally{$env:PYTHONPATH=$prior}
}
