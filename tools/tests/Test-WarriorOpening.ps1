param([switch]$ScopeOnly,[switch]$ExportOnly,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
function Shape([string]$s){([regex]::Replace($s,'(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
$path='src/server/scripts/Scenario/Artifacts/Warrior/Intro/wars_intro_scenario.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$old=(& git -C $repo show "77ebf820e:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned opening baseline source missing'}
& "$PSScriptRoot/Test-WarriorRitual.ps1" -ScopeOnly
$ritualCertified=(& git -C $repo show "1531780a2:$path") -join "`n";if($LASTEXITCODE){throw 'Certified pre-ritual actors missing'}
$source=$source.Replace((Block $source 'class npc_danica_the_reclaimer :'),(Block $ritualCertified 'class npc_danica_the_reclaimer :')).Replace('#include "ritual_transition.h"','')
$actor=Block $source 'class npc_ragnvald_dragonborn :';$previous=Block $old 'class npc_ragnvald_dragonborn :'
$los=Block $actor 'void MoveInLineOfSight(';$oldLos=Block $previous 'void MoveInLineOfSight('
if((Shape $los) -cne 'voidMoveInLineOfSight(Unit*)override{}'){throw 'Keep scripted auto-aggro suppression while instance owns the approach'}
$branch=Block $actor 'if (action == WarriorOpening::StartAction)';$queued=Block $branch 'presentationFunctions.AddDelayedEvent(2000,';$oldQueued=Block $oldLos 'presentationFunctions.AddDelayedEvent(2000,'
function Battle([string]$s){$s.Substring($s.IndexOf('std::list<Creature*> adds;')).TrimEnd().TrimEnd('}').Trim()}
if((Shape (Battle $queued)) -cne (Shape (Battle $oldQueued))){throw 'Original2s battle targeting/home/native attack helper body changed'}
$normal=$actor.Replace($los,$oldLos).Replace((Block $actor 'uint32 GetData('),'').Replace($branch,'').Replace('openingPending','introDone1').Replace('openingActorWarned','introDone2')
if((Shape $normal) -cne (Shape $previous)){throw 'Other Ragnvald timer, lifecycle, reset, handoff, second battle or combat changed'}
$normal=$source.Replace($actor,$previous)
foreach($header in @('Conversation.h','InstanceScript.h','opening_transition.h')){$normal=$normal.Replace('#include "'+$header+'"','')}
if((Shape $normal) -cne (Shape $old)){throw 'Other Warrior/Danica/Bezzeredes/Malgalor behavior changed'}
$instancePath='src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/instance_broken_shore.cpp';$actual=(Get-Content "$repo/$instancePath" -Raw).Replace("`r",'');$prior=(& git -C $repo show "77ebf820e:$instancePath") -join "`n";if($LASTEXITCODE){throw 'Pinned opening shared instance missing'}
$actual=(& git -C $repo show "1531780a2:$instancePath") -join "`n";if($LASTEXITCODE){throw 'Certified pre-ritual shared instance missing'}
$expected=$prior.Replace('#include "../../Warrior/Intro/first_wave_transition.h"','#include "../../Warrior/Intro/first_wave_transition.h"'+"`n"+'#include "../../Warrior/Intro/opening_transition.h"').Replace('warriorFirstWave(map) {}','warriorFirstWave(map), warriorOpening(map) {}').Replace('WarriorFirstWave::State warriorFirstWave;','WarriorFirstWave::State warriorFirstWave;'+"`n"+'WarriorOpening::State warriorOpening;').Replace('warriorTransition.Observe(creature);','warriorTransition.Observe(creature);'+"`n"+'warriorOpening.Observe(creature);').Replace("void SetData(uint32 type, uint32 data) override`n        {","void SetData(uint32 type, uint32 data) override`n        {`n            if (type == WarriorOpening::ClaimData) { warriorOpening.Claim(data); return; }").Replace("uint32 GetData(uint32 type) const override`n        {","uint32 GetData(uint32 type) const override`n        {`n            if (type == WarriorOpening::RequestData) return warriorOpening.Request();`n            if (type == WarriorOpening::StartedData) return warriorOpening.started ? warriorOpening.epoch : 0;").Replace("ObjectGuid GetGuidData(uint32 type) const override`n        {","ObjectGuid GetGuidData(uint32 type) const override`n        {`n            if (type == WarriorOpening::SourceData) return warriorOpening.source;").Replace('warriorTransition.Update(diff); warriorFirstWave.Update(diff);','warriorOpening.Update(diff); warriorTransition.Update(diff); warriorFirstWave.Update(diff);')
if((Shape $actual) -cne (Shape $expected)){throw 'Other shared Paladin/Warrior instance dispatch changed'}
if($ScopeOnly){'PASS instance-owned opening boundary, empty scripted LOS override, exact original2s battle payload and all remaining actor/Paladin/first-wave/reinforcement/finale bodies retained';return}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/warrior-opening"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
[IO.File]::WriteAllText("$out/Actor.inc",$actor+';')
[IO.File]::WriteAllText("$out/BaselineActor.inc",$previous.Replace('npc_ragnvald_dragonborn','npc_ragnvald_dragonborn_baseline')+';')
Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Warrior/Intro/opening_transition.h" "$out/Opening.inc"
Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Warrior/Intro/first_wave_transition.h" "$out/FirstWave.inc"
$helper=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Warrior/Intro/malgalor_finale.h" -Raw
[IO.File]::WriteAllText("$out/Original.inc",'namespace MalgalorFinale {'+(Block $helper 'struct Member')+';'+(Block $helper 'inline uint32 Quest(')+(Block $helper 'inline Player* Original(')+'}')
[IO.File]::WriteAllText("$out/QuestDelay.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::AddQuestDelayedEvent('))
$fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw
[IO.File]::WriteAllText("$out/UnitTimer.inc",(Block $unit 'void AddDelayedEvent('));[IO.File]::WriteAllText("$out/DeathState.inc",(Block $unit 'enum DeathState')+';')
$native=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
foreach($pair in @(@('Lifecycle','if (i_AI || i_disabledAI)'),@('AIDestroy','bool Creature::AIM_Destroy('))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $native $pair[1]))}
$castMacro=Get-Content "$repo/src/server/game/AI/ScriptedAI/ScriptedCreature.h"|Where-Object {$_ -match '^#define CAST_AI\('};if(@($castMacro).Count -ne 1){throw 'Native CAST_AI missing'};[IO.File]::WriteAllText("$out/Casts.inc",$castMacro)
[IO.File]::WriteAllText("$out/NativeVisibility.inc",(Block (Get-Content "$repo/src/server/game/Grids/Notifiers/GridNotifiers.cpp" -Raw) 'bool shouldCallMoveInLineOfSight('))
if($ExportOnly){return}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/WarriorOpeningTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Warrior opening extracted production ASAN regression failed'}
