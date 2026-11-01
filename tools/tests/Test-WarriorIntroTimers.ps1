param([switch]$Baseline,[switch]$ScopeOnly,[switch]$ExportOnly,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$text,[string]$start){$a=$text.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$text.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $text.Length){$depth+=([int]($text[$b] -eq '{'))-([int]($text[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$text.Substring($a,$b-$a)}
function Shape([string]$text){($text -replace '//[^\n]*','') -replace '\s',''}
$path='src/server/scripts/Scenario/Artifacts/Warrior/Intro/wars_intro_scenario.cpp'
$actualSource=(Get-Content "$repo/$path" -Raw).Replace("`r",'')
& "$PSScriptRoot/Test-WarriorIntroHandoffs.ps1" -ScopeOnly
$source=(& git -C $repo show "783373fdb:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-handoff timer implementation missing'}
$old=(& git -C $repo show "0ab5e6990:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-timer source missing'}
$normalized=$source.Replace('#include "FunctionProcessor.h"','');$expectedLife='void UpdateLifecycle(uint32 diff) override { if (me->getDeathState() == ALIVE || me->getDeathState() == CORPSE) presentationFunctions.Update(diff); }'
$expectedAttack=@'
static void AttackBattleTarget(Creature* ally, ObjectGuid targetGuid, CreatureAI* fallback = nullptr, uint8 remaining = 10) {
 Creature* target = ObjectAccessor::GetCreature(*ally, targetGuid);
 if (!ally->IsAlive() || !target || !target->IsAlive()) return;
 CreatureAI* ai = ally->AI(); if (!ai) ai = fallback;
 if (ai) { ai->AttackStart(target); return; }
 if (!remaining) { TC_LOG_ERROR("scripts", "Warrior intro: ally %s has no AI for its delayed attack after 10 retries", ally->GetGUID().ToString().c_str()); return; }
 ally->AddDelayedEvent(500, [ally, targetGuid, remaining]() -> void { AttackBattleTarget(ally, targetGuid, nullptr, remaining - 1); });
}
'@
foreach($actor in @('class npc_ragnvald_dragonborn :','class npc_danica_the_reclaimer :')){
 $body=Block $source $actor;$previous=Block $old $actor;$life=Block $body 'void UpdateLifecycle('
 if((Shape $life) -cne (Shape $expectedLife) -or ([regex]::Matches($body,'presentationFunctions.AddDelayedEvent\(')).Count -ne 1 -or $body.Contains('me->AddDelayedEvent(')){throw 'Each actual AI must own exactly one timer on its native ALIVE/CORPSE lifecycle clock'}
 $restored=$body.Replace('FunctionProcessor presentationFunctions;','').Replace($life,'').Replace('presentationFunctions.AddDelayedEvent(','me->AddDelayedEvent(')
 if($actor -eq 'class npc_ragnvald_dragonborn :'){
  $attack=Block $body 'static void AttackBattleTarget('
  if((Shape $attack) -cne (Shape $expectedAttack) -or ([regex]::Matches($body,'AttackBattleTarget\(attacker, targ->GetGUID\(\)\);')).Count -ne 2){throw 'Exact original-target/current-AI/fallback-only-self bounded attack helper changed'}
  $restored=$restored.Replace($attack,'').Replace('AttackBattleTarget(*itr, targ->GetGUID(), *itr == me ? this : nullptr);','(*itr)->AI()->AttackStart(targ);').Replace('AttackBattleTarget(attacker, targ->GetGUID());','attacker->AI()->AttackStart(targ);')
 }
 if((Shape $restored) -cne (Shape $previous)){throw "Other actor behavior changed: $actor"}
 $normalized=$normalized.Replace($body,$previous)
}
if((Shape $normalized) -cne (Shape $old)){throw 'Other Warrior/Malgalor/Bezzeredes behavior changed'}
if($ScopeOnly){'PASS exactly two AI-owned original2/3s timers/native ALIVE-CORPSE lifecycle and bounded original-target/current-AI battle recovery; all remaining actor/progress/cast/scene/motion bodies unchanged';return}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/warrior-intro-timers$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$selected=if($Baseline){$old}else{$actualSource}
[IO.File]::WriteAllText("$out/Actors.inc",(Block $selected 'class npc_ragnvald_dragonborn :')+";`n"+(Block $selected 'class npc_danica_the_reclaimer :')+';')
$fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw
[IO.File]::WriteAllText("$out/UnitTimer.inc",(Block $unit 'void AddDelayedEvent('))
[IO.File]::WriteAllText("$out/DeathState.inc",(Block $unit 'enum DeathState')+';')
[IO.File]::WriteAllText("$out/Talk.inc",(Block (Get-Content "$repo/src/server/game/AI/CreatureAI.cpp" -Raw) 'void CreatureAI::Talk(uint8 id, ObjectGuid WhisperGuid)'))
$castMacro=Get-Content "$repo/src/server/game/AI/ScriptedAI/ScriptedCreature.h"|Where-Object {$_ -match '^#define CAST_AI\('};if(@($castMacro).Count -ne 1){throw 'Native CAST_AI macro missing'}
[IO.File]::WriteAllText("$out/Casts.inc",$castMacro)
$native=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
foreach($pair in @(@('Lifecycle','if (i_AI || i_disabledAI)'),@('AIDestroy','bool Creature::AIM_Destroy('))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $native $pair[1]))}
if($ExportOnly){return}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/WarriorIntroTimersTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Warrior intro timer regression failed'}
