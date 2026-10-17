$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-StormheimShallowsData.ps1"
& "$PSScriptRoot/Test-StormheimJournals.ps1" *> "$repo/.codex/shallows-upstream.log"
$out="$repo/build-extractors/runtime-test/stormheim-shallows"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-journals/*.inc" | Copy-Item -Destination $out
$f=Get-Content "$out/HelheimSuppliesFixture.inc" -Raw
$f=$f.Replace('struct GameObject:WorldObject {','struct GameObject:WorldObject {GameObjectTemplate* data=nullptr;GameObjectTemplate const* GetGOInfo(){return data;}uint32 garrBuildingType=0;')
$f=$f.Replace('struct Group {','struct Group {template<class T>void UpdateLooterGuid(T*,bool=true){}')
$f=$f.Replace('struct ItemStore {','struct ItemStore {PersonalLootData const* GetPersonalLootData(uint32){return nullptr;}')
$f=$f.Replace('struct Player:Unit {','struct Player:Unit {void TestFill(GameObject*,Loot*,Loot*,bool=false);')
$f=$f.Replace('GetFlags()const{return 67584;}','GetFlags()const{return 2048;}').Replace('GetFlags2()const{return 8192;}','GetFlags2()const{return 12288;}')
[IO.File]::WriteAllText("$out/HelheimSuppliesFixture.inc",$f)
$f=Get-Content "$out/BuiltScaleFixture.inc" -Raw
$f=$f.Replace('struct {uint32 chestLoot=0;} chest,fishingHole,gatheringNode,challengeModeReward;','struct {uint32 chestLoot=0,usegrouplootrules=0,MinGold=0,MaxGold=0;} chest,fishingHole,gatheringNode,challengeModeReward;uint32 MinGold=0,MaxGold=0;')
$f=$f.Replace('struct Loot {','struct Loot {uint32 fills=0,dungeonEncounterID=0,objType=0,shipmentBuildingType=0;bool personal=false;void clear(){generated.clear();}void generateMoneyLoot(uint32,uint32,bool){}void FillLoot(uint32,LootStore const&,Player*,bool,bool,GameObject*);')
$f=$f.Replace('Index[8]{0,13}','Index[8]{0,5}').Replace('id==2160','id==1691').Replace('MiscValue=13','MiscValue=5')
$f=$f.Replace('SpellCastResult CanOpenLock(uint32,uint32,SkillType&,int32&,int32&);','SpellCastResult CanOpenLock(uint32,uint32,SkillType&,int32&,int32&);int effectHandleMode=1;Unit* unitTarget=nullptr;void EffectKillCredit(SpellEffIndex);')
[IO.File]::WriteAllText("$out/BuiltScaleFixture.inc",$f)
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$start=$core.IndexOf('void Player::SendLoot(ObjectGuid')
$a=$core.IndexOf('            if (lootid)',$start);$b=$core.IndexOf('            if (loot_type == LOOT_FISHING)',$a)
if($a -lt 0 -or $b -le $a){throw 'Native SendLoot fill branch missing'}
[IO.File]::WriteAllText("$out/ShallowsFill.inc",$core.Substring($a,$b-$a))
$effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
$a=$effects.IndexOf('void Spell::EffectKillCredit(');$b=$effects.IndexOf('void Spell::EffectQuestFail(',$a)
if($a -lt 0 -or $b -le $a){throw 'Native credit effect missing'}
[IO.File]::WriteAllText("$out/ShallowsCredit.inc",$effects.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimShallowsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Shallows native progression regression failed'}
