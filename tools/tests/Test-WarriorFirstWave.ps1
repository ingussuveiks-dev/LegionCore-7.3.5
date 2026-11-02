param([switch]$ScopeOnly,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
function Shape([string]$s){($s -replace '//[^\n]*','') -replace '\s',''}
$path='src/server/scripts/Scenario/Artifacts/Warrior/Intro/wars_intro_scenario.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$old=(& git -C $repo show "b37138290:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned first-wave source missing'}
$oldDanica=Block (Block $old 'class npc_danica_the_reclaimer :') 'void IsSummonedBy('
$legacy=Block $oldDanica 'if (Player* targ = me->FindNearestPlayer(100.0f))'
if((Shape $source) -cne (Shape $old.Replace($legacy,''))){throw 'First-wave change must remove only Danica summon-owned46878/47474; all actual opening/ritual/combat/timers remain unchanged'}
$instancePath='src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/instance_broken_shore.cpp'
$actual=(Get-Content "$repo/$instancePath" -Raw).Replace("`r",'');$oldInstance=(& git -C $repo show "b37138290:$instancePath") -join "`n";if($LASTEXITCODE){throw 'Pinned shared instance missing'}
$expected=$oldInstance.Replace('#include "../../Warrior/Intro/reinforcement_transition.h"','#include "../../Warrior/Intro/reinforcement_transition.h"'+"`n"+'#include "../../Warrior/Intro/first_wave_transition.h"').Replace('warriorTransition(map) {}','warriorTransition(map), warriorFirstWave(map) {}').Replace('WarriorReinforcement::State warriorTransition;','WarriorReinforcement::State warriorTransition;'+"`n"+'WarriorFirstWave::State warriorFirstWave;').Replace('warriorTransition.Death(creature, killer);','warriorFirstWave.Death(creature, killer);'+"`n"+'warriorTransition.Death(creature, killer);').Replace('warriorTransition.Update(diff);','warriorTransition.Update(diff); warriorFirstWave.Update(diff);')
if((Shape $expected) -cne (Shape $actual)){throw 'Shared instance changed beyond one owned first-wave state/constructor/death/update; preserve all Paladin and reinforcement/finale hooks'}
if($ScopeOnly){'PASS exact Danica nearest-player death-credit removal and actual shared-instance owned first-wave death/update hooks; all other Warrior and Paladin behavior preserved';return}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/warrior-first-wave"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Warrior/Intro/first_wave_transition.h" "$out/FirstWave.inc"
$oldFirstWave=(& git -C $repo show '45315534e:src/server/scripts/Scenario/Artifacts/Warrior/Intro/first_wave_transition.h') -join "`n";if($LASTEXITCODE){throw 'Pinned early-death first-wave baseline missing'}
[IO.File]::WriteAllText("$out/BaselineFirstWave.inc",$oldFirstWave.Replace('namespace WarriorFirstWave','namespace WarriorFirstWaveBaseline'))
$helper=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Warrior/Intro/malgalor_finale.h" -Raw
[IO.File]::WriteAllText("$out/Original.inc",'namespace MalgalorFinale {'+(Block $helper 'struct Member')+';'+(Block $helper 'inline uint32 Quest(')+(Block $helper 'inline Player* Original(')+'}')
[IO.File]::WriteAllText("$out/Legacy.inc",$legacy)
$player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
[IO.File]::WriteAllText("$out/QuestDelay.inc",(Block $player 'void Player::AddQuestDelayedEvent('))
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw;$kill=Block $unit 'void Unit::Kill('
$start=$kill.IndexOf('        // Call creature just died function');$end=$kill.IndexOf('        // Dungeon specific stuff',$start)
if($start -lt 0 -or $end -le $start){throw 'Native SmartAI/summon/instance death order absent'}
[IO.File]::WriteAllText("$out/DeathOrder.inc",$kill.Substring($start,$end-$start))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/WarriorFirstWaveTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Warrior first-wave extracted production ASAN regression failed'}
