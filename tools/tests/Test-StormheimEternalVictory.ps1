param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimEternalVictoryData.ps1"
& "$PSScriptRoot/Test-StormheimEternalNemesis.ps1" *> "$PSScriptRoot/../../.codex/victory-upstream.log"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-eternal-victory"
$previous="$repo/build-extractors/runtime-test/stormheim-eternal-nemesis"
Get-ChildItem "$previous/*.inc" | Copy-Item -Destination $output
$fixture=Get-Content "$previous/NemesisFixture.inc" -Raw
$fixture=$fixture.Replace('struct Creature:Unit {','struct Creature:Unit {Position home;Position const& GetHomePosition(){return home;}std::set<ObjectGuid> rewarded;void AddRewardPlayer(ObjectGuid guid){rewarded.insert(guid);}bool rewardAllowed=true;bool IsDamageEnoughForLootingAndReward(){return rewardAllowed;}')
$fixture=$fixture.Replace('struct CreatureTemplate {','struct CreatureTemplate {uint32 Entry=91948,KillCredit[2]{};')
$fixture=$fixture.Replace('struct Creature:Unit {','struct Creature:Unit {void SetLootRecipient(Unit*){}')
$fixture=$fixture.Replace('struct Player:Unit {','struct Player:Unit {void KilledMonster(CreatureTemplate const*,ObjectGuid);std::vector<uint64> delays;')
$fixture=$fixture.Replace('struct Player:Unit {','struct Player:Unit {bool contact=true;bool CanContact(){return contact&&world;}')
$fixture=$fixture.Replace('void AddDelayedEvent(uint64,std::function<void()> f){events.push_back(std::move(f));}', 'void AddDelayedEvent(uint64 delay,std::function<void()> f){delays.push_back(delay);events.push_back(std::move(f));}')
$fixture=$fixture.Replace('virtual void JustDied(Unit*){++delegated;}','NativeEvents* GetScript();DeathInstance* instance=nullptr;uint32 _bossId=0;bool HasEscortState(int){return false;}void EndPath(bool){}virtual void JustDied(Unit*);')
[IO.File]::WriteAllText("$output/VictoryFixture.inc",$fixture)
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$core.IndexOf('void Player::KilledMonster(');$b=$core.IndexOf('void Player::KilledMonsterCredit(',$a)
[IO.File]::WriteAllText("$output/NativeKilledMonster.inc",$core.Substring($a,$b-$a))
$reward=Get-Content "$repo/src/server/game/Entities/Player/KillRewarder.cpp" -Raw
$a=$reward.IndexOf('inline void KillRewarder::_RewardKillCredit(');$b=$reward.IndexOf('void KillRewarder::_RewardPlayer(',$a)
[IO.File]::WriteAllText("$output/NativeKillReward.inc",$reward.Substring($a,$b-$a))
$a=$reward.IndexOf('            for (GroupReference* itr = _group->GetFirstMember();',$reward.IndexOf('void KillRewarder::_RewardGroup('))
$b=$reward.IndexOf('_RewardPlayer(member, isDungeon);',$a)+'_RewardPlayer(member, isDungeon);'.Length
[IO.File]::WriteAllText("$output/NativeGroupSelection.inc",$reward.Substring($a,$b-$a))
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$a=$unit.IndexOf('    // find player: owner of controlled',$unit.IndexOf('void Unit::Kill('));$b=$unit.IndexOf('    // Reward player, his pets, and group/raid members',$a)
[IO.File]::WriteAllText("$output/KillOwner.inc",$unit.Substring($a,$b-$a))
$ai=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
$a=$ai.IndexOf('void SmartAI::JustDied(');$b=$ai.IndexOf('void SmartAI::KilledUnit(',$a)
[IO.File]::WriteAllText("$output/NativeSmartDeath.inc",$ai.Substring($a,$b-$a))
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_geir.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$output/Geir.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_geir()')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$output`" `"$PSScriptRoot/StormheimEternalVictoryTest.cpp`" /Fe`"$output/EternalVictory.exe`" /Fo`"$output/EternalVictory.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/EternalVictory.exe`" `"$output/world.tsv`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Eternal Victory native regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe";$bytes=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 foreach($marker in @('npc_stormheim_geir')){if(!$bytes.Contains($marker)){throw "Canonical Release lacks $marker"}}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 foreach($name in @('scripts','game')){
  $relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match "$name\\Release\\$name.lib$"} | Select-Object -First 1
  if(!$relative){throw "Missing actual $name link input"}
  $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
  if(!(Test-Path $lib) -or (Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw "Canonical Release predates $name link input"}
 }
 'PASS: canonical Release Geir binding and actual scripts/game final-link inputs.'
}
