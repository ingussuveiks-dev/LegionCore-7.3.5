param([switch]$PrepareHost,[switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
$path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$old=(& git -C $repo show "c3dd68ad5:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-Erunak-timer source missing'}
$newEntry=$source.Contains('#include "sharasdal_erunak_entry.h"')
if($newEntry){
 & "$PSScriptRoot/Test-SharasdalErunakEntry.ps1" -ScopeOnly
 if(!$?){throw 'Current lower Erunak entry certificate failed'}
 # This older timer matrix remains pinned after the exact entry certificate;
 # the separate entry suite compiles the full current actor and owned timer.
 $source=(& git -C $repo show "bf9292b2f:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned retained timer actor missing'}
}
$npc=Block $source 'class npc_erunak_stonespeaker :';$oldNPC=Block $old 'class npc_erunak_stonespeaker :'
$life=Block $npc 'void UpdateLifecycle('
$expected=@'
void UpdateLifecycle(uint32 diff) override
        {
            SharasdalCompanion::Recover(me, _pendingHealRecovery, wait_heal);
            // AI-owned intro timers retain the Unit clock while stunned or suspended.
            _functions.Update(diff);
        }
'@
if(($life -replace '\s','') -cne ($expected -replace '\s','')){throw 'Erunak actual lifecycle clock/recovery changed'}
$normal=$npc.Replace($life,(Block $oldNPC 'void UpdateLifecycle(')).Replace('_functions.AddDelayedEvent(5000, [this]() -> void','me->AddDelayedEvent(5000, [this]() -> void').Replace('_functions.AddDelayedEvent(3000, [this]() -> void','me->AddDelayedEvent(3000, [this]() -> void').Replace("            if (!UpdateVictim())","            _functions.Update(diff);`n`n            if (!UpdateVictim())")
$normal=$normal.Replace("                        if (CreatureAI* ai = me->AI())`n                            ai->AttackStart(targ);",'                        me->AI()->AttackStart(targ);')
if(($normal -replace '\s','') -cne ($oldNPC -replace '\s','')){throw 'Other Erunak admission/roles/credit/paths/spells/reset/wounds changed'}
if(($source.Replace($npc,$oldNPC) -replace '\s','') -cne ($old -replace '\s','')){throw 'Other companions/bosses/loot/registrations changed'}
if($ScopeOnly){'PASS only Erunak3s/5s AI-owned queue and once-per-native-lifecycle clock; no admission/path/credit/reset/wound/other-actor changes';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-erunak-timers$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
[IO.File]::WriteAllText("$out/Actor.inc",$(if($Baseline){$oldNPC}else{$npc})+';')
[IO.File]::WriteAllText("$out/Recovery.inc",(Block $source 'namespace SharasdalCompanion'))
$fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
$eh=(Get-Content "$repo/src/server/game/AI/EventMap.h" -Raw).Replace('#include "Define.h"','').Replace('#include "Duration.h"','').Replace('TC_GAME_API ','')
$es=(Get-Content "$repo/src/server/game/AI/EventMap.cpp" -Raw).Replace('#include "EventMap.h"','').Replace('#include "Random.h"','')
[IO.File]::WriteAllText("$out/EventMap.inc",$eh+$es)
$cs=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
foreach($pair in @(@('Lifecycle','if (i_AI || i_disabledAI)'),@('AIDestroy','bool Creature::AIM_Destroy('))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $cs $pair[1]))}
[IO.File]::WriteAllText("$out/SafeLOS.inc",(Block (Get-Content "$repo/src/server/game/AI/CreatureAI.cpp" -Raw) 'void CreatureAI::MoveInLineOfSight_Safe('))
if($PrepareHost){return}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SharasdalErunakTimersTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Actual full Erunak timer regression failed'}
if($newEntry){'Retained42 timer matrix uses pinned bf9292b2f after strict lower-entry certificate; current full actor/entry/clock is exercised by the separate entry suite'}
if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$previous=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sharasdal_erunak_timers_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Actual Erunak timer Release implementation missing'}}finally{$env:PYTHONPATH=$previous}}
