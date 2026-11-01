param([switch]$Baseline,[switch]$ScopeOnly,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$text,[string]$start){$a=$text.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$text.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $text.Length){$depth+=([int]($text[$b] -eq '{'))-([int]($text[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$text.Substring($a,$b-$a)}
function Shape([string]$text){($text -replace '//[^\n]*','') -replace '\s',''}
$path='src/server/scripts/Scenario/Artifacts/Warrior/Intro/wars_intro_scenario.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$old=(& git -C $repo show "783373fdb:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-handoff source missing'}
$expected=@'
static void ResumeBattle(Creature* ragnvald, uint8 remaining = 10) {
 if (!ragnvald->IsInWorld() || ragnvald->IsPreDelete() || !ragnvald->IsAlive() || ragnvald->GetEntry() != 97167 || ragnvald->GetMapId() != 1500) return;
 Scenario* scenario = sScenarioMgr->GetScenario(ragnvald->GetInstanceId());
 if (scenario && (scenario->GetScenarioId() != 933 || scenario->GetCurrentStep() < 2 || scenario->GetCurrentStep() > 3)) return;
 if (scenario && scenario->GetCurrentStep() == 3) { if (auto* ai = CAST_AI(npc_ragnvald_dragonbornAI, ragnvald->AI())) { ai->DoAction(true); return; } }
 if (!remaining) { TC_LOG_ERROR("scripts", "Warrior intro: Ragnvald %s is missing native933 stage3 or matching AI after 10 battle handoff retries", ragnvald->GetGUID().ToString().c_str()); return; }
 uint32 instanceId = ragnvald->GetInstanceId();
 ragnvald->AddDelayedEvent(500, [ragnvald, remaining, instanceId]() -> void { if (ragnvald->GetInstanceId() == instanceId) ResumeBattle(ragnvald, remaining - 1); });
}
'@
$helper=Block $source 'static void ResumeBattle('
if((Shape $helper) -cne (Shape $expected)){throw 'Expected typed original-Ragnvald Unit-owned finite handoff changed'}
$normal=$source.Replace($helper,'').Replace('npc_ragnvald_dragonborn::npc_ragnvald_dragonbornAI::ResumeBattle(targ);','targ->AI()->DoAction(true);').Replace('sCreatureTextMgr->SendChat(targ, 0);','targ->AI()->Talk(0);')
$ragnvald=Block $normal 'class npc_ragnvald_dragonborn :';$ragnvaldOld=Block $old 'class npc_ragnvald_dragonborn :'
$ragnvald=$ragnvald.Replace('AttackBattleTarget(*itr, targ->GetGUID());','(*itr)->AI()->AttackStart(targ);')
if((Shape $ragnvald) -cne (Shape $ragnvaldOld)){throw 'Other Ragnvald timer, retry, fallback, native scene/combat or admission changed'}
$normal=$normal.Replace((Block $normal 'class npc_ragnvald_dragonborn :'),$ragnvaldOld)
$normal=$normal.Replace('npc_ragnvald_dragonborn::npc_ragnvald_dragonbornAI::AttackBattleTarget(*itr, targ->GetGUID());','(*itr)->AI()->AttackStart(targ);')
if((Shape $normal) -cne (Shape $old)){throw 'Other Warrior intro/Bezzeredes/Malgalor behavior changed'}
if($ScopeOnly){'PASS exact two immediate foreign-ally attack handoffs, native97166 Talk0 forward and typed original-Ragnvald500ms/10 DoAction retry; all other timer, scene, criteria, combat and shared actor bodies retained';return}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/warrior-intro-handoffs$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Test-WarriorIntroTimers.ps1" -ExportOnly -OutputDirectory $out
$selected=if($Baseline){$old}else{$source}
if($Baseline -and (([regex]::Matches($old,'\(\*itr\)->AI\(\)->AttackStart\(targ\);')).Count -ne 2 -or !$old.Contains('targ->AI()->Talk(0);') -or !$old.Contains('targ->AI()->DoAction(true);') -or $old.Contains('static void ResumeBattle('))){throw 'Pinned unsafe immediate-handoff controls changed'}
[IO.File]::WriteAllText("$out/Actors.inc",((@('class npc_ragnvald_dragonborn :','class npc_danica_the_reclaimer :','class npc_bezzered :')|ForEach-Object {(Block $selected $_)+';'}) -join "`n"))
$support=(Get-Content "$PSScriptRoot/WarriorIntroTimersTest.cpp" -Raw).Replace("`r",'');$support=$support.Substring(0,$support.IndexOf('int main('))
[IO.File]::WriteAllText("$out/Support.inc",$support)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run-handoff.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/WarriorIntroHandoffsTest.cpp`" /Fe`"$out/handoff.exe`" /Fo`"$out/handoff.obj`" /Fd`"$out/handoff.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/handoff.exe`"`nexit /b %errorlevel%`n")
& "$out/run-handoff.cmd";if($LASTEXITCODE){throw 'Native Warrior immediate handoff regression failed'}
