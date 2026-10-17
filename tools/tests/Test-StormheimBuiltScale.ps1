$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-StormheimBuiltScaleData.ps1"
& "$PSScriptRoot/Test-StormheimHelheimSupplies.ps1" *> "$repo/.codex/built-scale-upstream.log"
$out="$repo/build-extractors/runtime-test/stormheim-built-scale"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-helheim-supplies/*.inc" | Copy-Item -Destination $out
$fixture=Get-Content "$out/HelheimSuppliesFixture.inc" -Raw
$fixture=$fixture.Replace('struct ItemTemplate {','struct ItemTemplate {uint32 start=0,stack=50,FlagsCu=0;uint32 GetStartQuestID()const{return start;}uint32 GetFlags()const{return 67584;}uint32 GetFlags2()const{return 8192;}uint32 GetRequiredSkill()const{return 0;}struct Effect{uint32 SpellID;};std::vector<Effect*> Effects;')
$fixture=$fixture.Replace('GetMaxStackSize()const{return 10;}','GetMaxStackSize()const{return stack;}')
$fixture=$fixture.Replace('ItemTemplate item;ItemTemplate const* GetItemTemplate(uint32){return &item;}','std::map<uint32,ItemTemplate> items;ItemTemplate const* GetItemTemplate(uint32 id){auto i=items.find(id);return i==items.end()?nullptr:&i->second;}')
$fixture=$fixture.Replace('bool IsDFQuest()const','bool IsDaily()const{return false;}bool IsWorld()const{return false;}bool IsDFQuest()const')
$fixture=$fixture.Replace('int GetQuestStatus(uint32 q){auto s=getQuestStatus(q);return s?s->Status:QUEST_STATUS_NONE;}','QuestStatus GetQuestStatus(uint32)const;')
$fixture=$fixture.Replace('bool CanTakeQuest(Quest const* q,bool b){return SatisfyQuestRace(q,b)&&SatisfyQuestPreviousQuest(q,b);}','bool CanTakeQuest(Quest const*,bool);bool SatisfyQuestStatus(Quest const*,bool);')
$gates=''
foreach($gate in @('ExclusiveGroup','Class','Level','Skill','Reputation','Timed','NextChain','PrevChain','Day','Conditions')){$gates+="bool SatisfyQuest$gate(Quest const*,bool){return true;}"}
$fixture=$fixture.Replace('struct Player:Unit {','struct StarterItem;struct Player:Unit {'+$gates+'bool SatisfyQuestWeek(Quest const*){return true;}bool SatisfyQuestSeasonal(Quest const*){return true;}bool WorldQuestCompleted(uint32)const{return false;}void PickUp(StarterItem*);bool HasSkill(uint32){return false;}bool HasSpell(uint32){return false;}Garrison* GetGarrison(){return nullptr;}bool isGameMaster()const{return false;}')
$fixture=$fixture.Replace('uint32 GetTeam(){return team;}','uint32 GetTeam()const{return team;}')
foreach($method in @('IsQuestDailyRewarded','IsQuestWeekRewarded','IsQuestSeasonalRewarded')){$fixture=$fixture.Replace("bool $method(uint32){", "bool $method(uint32)const{")}
$fixture=$fixture.Replace('void SendQuestGiverQuestDetails(Quest const* q,ObjectGuid,bool,bool)','void SendQuestGiverQuestDetails(Quest const* q,ObjectGuid,bool,bool,bool=false,uint32=0)')
[IO.File]::WriteAllText("$out/HelheimSuppliesFixture.inc",$fixture)
$prefix=Get-Content "$PSScriptRoot/StormheimHelheimSuppliesTest.cpp" -Raw
$prefix=$prefix.Substring(0,$prefix.IndexOf('int main('))
$prefix=$prefix.Replace('struct LootItem {','struct LootItem {bool AllowedForPlayer(Player const*)const{return true;}')
$prefix=$prefix.Replace('struct Loot {','struct Loot {bool AllowedForPlayer(Player const*,uint32,uint32,uint8,bool,LootItem* = nullptr)const;')
$prefix=$prefix.Replace('struct CurrencyTypesEntry{};','struct CurrencyTypesEntry{uint32 CategoryID=0;};')
[IO.File]::WriteAllText("$out/BuiltScaleFixture.inc",$prefix)
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
foreach($segment in @(@('BuiltScaleStatus','QuestStatus Player::GetQuestStatus(','QuestStatus Player::GetDailyQuestStatus('),@('BuiltScaleAdmission','bool Player::CanTakeQuest(','bool Player::CanAddQuest('),@('BuiltScaleStatusGate','bool Player::SatisfyQuestStatus(','bool Player::SatisfyQuestConditions('))){$a=$core.IndexOf($segment[1]);$b=$core.IndexOf($segment[2],$a);if($a -lt 0 -or $b -lt $a){throw 'Native quest admission segment missing'};[IO.File]::WriteAllText("$out/$($segment[0]).inc",$core.Substring($a,$b-$a))}
$a=$core.IndexOf('    uint32 startQuestId = item->GetTemplate()->GetStartQuestID();');$b=$core.IndexOf("`n}",$a)
if($a -lt 0 -or $b -lt $a){throw 'Native item-pickup quest start missing'}
[IO.File]::WriteAllText("$out/BuiltScalePickup.inc",$core.Substring($a,$b-$a))
$loot=Get-Content "$repo/src/server/game/Loot/LootMgr.cpp" -Raw
$a=$loot.IndexOf('bool Loot::AllowedForPlayer(');$b=$loot.IndexOf('// --------- LootTemplate::LootGroup ---------',$a)
[IO.File]::WriteAllText("$out/BuiltScaleAllowed.inc",$loot.Substring($a,$b-$a))
$item=Get-Content "$repo/src/server/game/Entities/Item/Item.cpp" -Raw
$a=$item.IndexOf('bool Item::hasQuest(');$b=$item.IndexOf('bool Item::hasInvolvedQuest(',$a)
[IO.File]::WriteAllText("$out/BuiltScaleHasQuest.inc",$item.Substring($a,$b-$a).Replace('Item::','StarterItem::'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimBuiltScaleTest.cpp`" /Fe`"$out/BuiltScale.exe`" /Fo`"$out/BuiltScale.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/BuiltScale.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Built to Scale native item/loot regression failed'}
