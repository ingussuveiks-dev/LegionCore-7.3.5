$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-StormheimOscillatorData.ps1"
& "$PSScriptRoot/Test-StormheimHelheimSupplies.ps1" *> "$repo/.codex/oscillator-upstream.log"
$out="$repo/build-extractors/runtime-test/stormheim-oscillator";$upstream="$repo/build-extractors/runtime-test/stormheim-helheim-supplies"
Get-ChildItem "$upstream/*.inc" | Copy-Item -Destination $out
$f=Get-Content "$out/HelheimSuppliesFixture.inc" -Raw
$f=$f.Replace('struct ItemTemplate {','struct ItemTemplate {uint32 GetItemNameDescriptionID()const{return 0;}uint32 GetClass()const{return 12;}bool IsCraftingReagent()const{return false;}bool IsRecipe()const{return false;}')
$f=$f.Replace('struct ItemStore {','struct PersonalLootData {uint32 chance=0;};struct ItemStore {PersonalLootData const* GetPersonalLootData(uint32){return nullptr;}')
[IO.File]::WriteAllText("$out/HelheimSuppliesFixture.inc",$f)
$prefix=Get-Content "$PSScriptRoot/StormheimHelheimSuppliesTest.cpp" -Raw
$prefix=$prefix.Substring(0,$prefix.IndexOf('int main('))
$prefix=$prefix.Replace('struct GameObjectTemplate {','struct GameObjectTemplate {uint32 entry=240286,MinGold=0,MaxGold=0;bool IsOploteChest()const{return false;}')
$prefix=$prefix.Replace('struct {uint32 chestLoot=0;} chest,fishingHole,gatheringNode,challengeModeReward;','struct {uint32 chestLoot=0,chestPersonalLoot=60251,DungeonEncounter=0,usegrouplootrules=0;} chest,fishingHole,gatheringNode,challengeModeReward;')
$prefix=$prefix.Replace('struct LootStore {','struct LootStore {bool IsRatesAllowed()const{return true;}')
$prefix=$prefix.Replace('struct Loot {','struct Loot {bool personal=false,isRareOrGo=true,isRareNext=false;uint32 objType=0,dungeonEncounterID=0,fillCalls=0;void clear(){items.clear();generated.clear();isBoss=false;}void generateMoneyLoot(uint32,uint32,bool){}void FillLoot(uint32,LootStore const&,Player*,bool,bool,void*);')
$prefix=$prefix.Replace('void ProcessAutoGroup(Loot&,LootTemplate const*)const{}','void ProcessAutoGroup(Loot&,LootTemplate const*)const{}void ProcessRareOrGoLoot(Loot&,LootTemplate const*)const{}')
$prefix=$prefix.Replace('LootGroups Groups,AutoGroups;','LootGroups Groups,AutoGroups,PersonalGroups;void ProcessRareOrGoLoot(Loot&)const;template<class... T>void ProcessBossLoot(T&&...)const{assert(false);}template<class... T>void ProcessItemLoot(T&&...)const{assert(false);}template<class... T>void ProcessOploteChest(T&&...)const{assert(false);}template<class... T>void ProcessChallengeChest(T&&...)const{assert(false);}')
$prefix=$prefix.Replace('id==2160?&value:nullptr','id==1690?&value:nullptr').Replace('Index[8]{0,13}','Index[8]{0,5}')
$prefix=$prefix.Replace('float getRate(int){','uint32 getNextInstanceWeeklyReset(){return 999999;}uint32 getNextInstanceDailyReset(){return 999999;}float getRate(int){')
[IO.File]::WriteAllText("$out/OscillatorFixture.inc",$prefix)
$loot=Get-Content "$repo/src/server/game/Loot/LootMgr.cpp" -Raw
$a=$loot.IndexOf('void LootTemplate::ProcessRareOrGoLoot(');$b=$loot.IndexOf('void LootTemplate::ProcessWorld(',$a)
[IO.File]::WriteAllText("$out/NativeRareLoot.inc",$loot.Substring($a,$b-$a))
$a=$loot.IndexOf('    if (tab)',$loot.IndexOf('bool Loot::FillLoot('));$b=$loot.IndexOf('    if (!isBoss && !strcmp',$a)
[IO.File]::WriteAllText("$out/NativeLootDispatch.inc",$loot.Substring($a,$b-$a))
$player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$player.IndexOf('            if (lootid)',$player.IndexOf('void Player::SendLoot('));$b=$player.IndexOf('                else' + "`r`n" + '                {' + "`r`n" + '                    loot->clear();',$a)
if($b -lt 0){$b=$player.IndexOf('                else' + "`n" + '                {' + "`n" + '                    loot->clear();',$a)}
if($a -lt 0 -or $b -lt 0){throw 'Native personal GO loot branch not found'}
[IO.File]::WriteAllText("$out/NativePersonalOpen.inc",$player.Substring($a,$b-$a).Replace('this, true, false, go','&player, true, false, go')+"}`n")
$a=$player.IndexOf('void Player::AddPlayerLootCooldown(');$b=$player.IndexOf('void Player::_LoadLfgCooldown(',$a)
[IO.File]::WriteAllText("$out/NativeLootCooldown.inc",$player.Substring($a,$b-$a).Replace('Player::','CooldownPlayer::'))
$go=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$a=$go.IndexOf('bool GameObject::IsPersonal() const');$b=$go.IndexOf('void GameObject::AddToSkillupList(',$a)
[IO.File]::WriteAllText("$out/NativePersonal.inc",$go.Substring($a,$b-$a).Replace('GameObject::','LootGameObject::'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimOscillatorTest.cpp`" /Fe`"$out/Oscillator.exe`" /Fo`"$out/Oscillator.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/Oscillator.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Oscillator native regression failed'}
