$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimBreakSpineData.ps1"
& "$PSScriptRoot/Test-StormheimValorDoor.ps1" *> "$PSScriptRoot/../../.codex/spine-upstream.log"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-break-spine"
$upstream="$repo/build-extractors/runtime-test/stormheim-valor-door"
Get-ChildItem "$upstream/*.inc" | Copy-Item -Destination $out
$f=Get-Content "$upstream/ValorFixture.inc" -Raw
$f=$f.Replace('struct CreatureTemplate {','struct CreatureTemplate {uint32 Entry=0,KillCredit[2]{};')
$f=$f.Replace('struct Creature:Unit {','struct Creature:Unit {std::set<ObjectGuid> rewarded;void AddRewardPlayer(ObjectGuid guid){rewarded.insert(guid);}bool rewardAllowed=true;bool IsDamageEnoughForLootingAndReward(){return rewardAllowed;}void SetLootRecipient(Unit*){}')
$f=$f.Replace('struct Player:Unit {','struct Player:Unit {void KilledMonster(CreatureTemplate const*,ObjectGuid);std::vector<uint64> delays;bool contact=true;bool CanContact(){return contact&&world;}')
$f=$f.Replace('void AddDelayedEvent(uint64,std::function<void()> f){events.push_back(std::move(f));}','void AddDelayedEvent(uint64 delay,std::function<void()> f){delays.push_back(delay);events.push_back(std::move(f));}')
[IO.File]::WriteAllText("$out/SpineFixture.inc",$f)
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$core.IndexOf('void Player::KilledMonster(');$b=$core.IndexOf('void Player::KilledMonsterCredit(',$a)
[IO.File]::WriteAllText("$out/NativeKilledMonster.inc",$core.Substring($a,$b-$a))
$reward=Get-Content "$repo/src/server/game/Entities/Player/KillRewarder.cpp" -Raw
$a=$reward.IndexOf('inline void KillRewarder::_RewardKillCredit(');$b=$reward.IndexOf('void KillRewarder::_RewardPlayer(',$a)
[IO.File]::WriteAllText("$out/NativeKillReward.inc",$reward.Substring($a,$b-$a))
$a=$reward.IndexOf('            for (GroupReference* itr = _group->GetFirstMember();',$reward.IndexOf('void KillRewarder::_RewardGroup('));$b=$reward.IndexOf('_RewardPlayer(member, isDungeon);',$a)+'_RewardPlayer(member, isDungeon);'.Length
[IO.File]::WriteAllText("$out/NativeGroupSelection.inc",$reward.Substring($a,$b-$a))
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$a=$unit.IndexOf('    // find player: owner of controlled',$unit.IndexOf('void Unit::Kill('));$b=$unit.IndexOf('    // Reward player, his pets, and group/raid members',$a)
[IO.File]::WriteAllText("$out/KillOwner.inc",$unit.Substring($a,$b-$a))
$creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
$a=$creature.IndexOf('bool CreatureSpell::CanUseInDifficulty(');$b=$creature.IndexOf('CreatureMovementData::CreatureMovementData(',$a)
[IO.File]::WriteAllText("$out/NativeSpellDifficulty.inc",$creature.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$out`" `"$PSScriptRoot/StormheimBreakSpineTest.cpp`" /Fe`"$out/BreakSpine.exe`" /Fo`"$out/BreakSpine.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/BreakSpine.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Break the Spine native regression failed'}
