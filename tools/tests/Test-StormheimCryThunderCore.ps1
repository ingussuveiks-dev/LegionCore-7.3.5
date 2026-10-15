$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-cry-thunder-core"
New-Item -ItemType Directory -Force -Path $output | Out-Null
& "$PSScriptRoot/Export-QuestDelayedEvent.ps1" -OutputDirectory $output
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
foreach($segment in @(
 @('NextQuest','Quest const* Player::GetNextQuest(','bool Player::CanSeeStartQuest('),
 @('Race','bool Player::SatisfyQuestRace(','bool Player::SatisfyQuestReputation('),
 @('KillCreditGO','void Player::KillCreditGO(','bool Player::TalkedToCreature('),
 @('CanRewardQuest','bool Player::CanRewardQuest(Quest const* quest, bool msg)','bool Player::CanRewardQuest(Quest const* quest, uint32 reward'),
 @('CanCompleteQuest','bool Player::CanCompleteQuest(','bool Player::CanCompleteRepeatableQuest('),
 @('PreviousQuest','bool Player::SatisfyQuestPreviousQuest(','bool Player::SatisfyQuestClass('),
 @('KilledMonsterCredit','void Player::KilledMonsterCredit(','void Player::KilledPlayerCredit('),
 @('GroupCredit','void Player::RewardPlayerAndGroupAtEvent(','bool Player::IsAtGroupRewardDistance('),
 @('QuestObjectiveSatisfy','void Player::QuestObjectiveSatisfy(','void Player::SetQuestStatus(')
)){
 $a=$core.IndexOf($segment[1]);$b=$core.IndexOf($segment[2],$a)
 if($a -lt 0 -or $b -lt 0){throw "Missing core handler $($segment[0])"}
 [IO.File]::WriteAllText("$output/$($segment[0]).inc",$core.Substring($a,$b-$a))
}
$loader=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
$a=$loader.IndexOf('        if (qinfo->PrevQuestID)');$b=$loader.IndexOf('        if (qinfo->ExclusiveGroup)',$a)
[IO.File]::WriteAllText("$output/QuestPrevLoader.inc",$loader.Substring($a,$b-$a))
& "$PSScriptRoot/Test-StormheimHeart.ps1" > "$output/heart-regression.log"
if($LASTEXITCODE){throw 'Previous Heart regression failed'}
Copy-Item "$repo/build-extractors/runtime-test/stormheim-heart/*.inc" $output
$fixture=Get-Content "$PSScriptRoot/StormheimHeartTest.cpp" -Raw
$fixture=$fixture.Substring(0,$fixture.IndexOf('int main()'))
$fixture=$fixture.Replace('static ObjectGuid const Empty;', 'explicit operator bool()const{return value!=0;}static ObjectGuid const Empty;')
$fixture=$fixture.Replace('bool TalkedToCreature(uint32,ObjectGuid){return false;}', 'bool TalkedToCreature(uint32,ObjectGuid);')
$fixture=$fixture.Replace('struct Creature:Unit {', 'struct Creature:Unit {uint32 m_templateSpells[8]{};bool IsAIEnabled=true;void KillAllDelayedCombats(){}')
$fixture=$fixture.Replace('struct SpellInfo {uint32 Id=180737;};', 'struct SpellInfo {uint32 Id=180737;bool IsPassive()const{return false;}};')
$fixture=$fixture.Replace('struct Unit:WorldObject {', 'struct Unit:WorldObject {void NativeDeath(Creature*);')
[IO.File]::WriteAllText("$output/CryCoreFixture.inc",$fixture)
$a=$core.IndexOf('bool Player::TalkedToCreature(');$b=$core.IndexOf('void Player::MoneyChanged(', $a)
[IO.File]::WriteAllText("$output/Talked.inc",$core.Substring($a,$b-$a))
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$a=$unit.IndexOf('        if (creature->IsAIEnabled)', $unit.IndexOf('// Call creature just died function'));$b=$unit.IndexOf('        if (TempSummon*', $a)
[IO.File]::WriteAllText("$output/NativeDeath.inc",$unit.Substring($a,$b-$a))
$a=$core.IndexOf('    for (uint32 i = 0; i < MAX_SPELL_CONTROL_BAR;', $core.IndexOf('void Player::VehicleSpellInitialize('));$b=$core.IndexOf('    time_t now', $a)
[IO.File]::WriteAllText("$output/VehicleBar.inc",$core.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$output`" `"$PSScriptRoot/StormheimCryThunderCoreTest.cpp`" /Fe`"$output/StormheimCryThunderCore.exe`" /Fo`"$output/StormheimCryThunderCore.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/StormheimCryThunderCore.exe`"`r`n"
[IO.File]::WriteAllText("$output/run-CryThunderCore.cmd",$cmd)
& "$output/run-CryThunderCore.cmd"
if($LASTEXITCODE){throw 'Cry Thunder native core regression failed'}
