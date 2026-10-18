$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/lost-army"
New-Item -ItemType Directory -Force $out | Out-Null
function Query([string]$sql,[string]$database='World'){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match "^${database}DatabaseInfo\s*="} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Lost Army SQL failed'}}finally{$env:MYSQL_PWD=$previous};$rows
}
$migration=Get-Content "$repo/sql/updates/world/2026_10_18_02_lost_army_world_quests.sql" -Raw
$select="SELECT SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=1 AND SourceEntry=142487 AND ConditionTypeOrReference=60 ORDER BY SourceGroup;"
$rows=@(Query @"
CREATE TEMPORARY TABLE saved_conditions AS SELECT * FROM conditions WHERE SourceTypeOrReferenceId=1;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions; INSERT INTO conditions SELECT * FROM saved_conditions;
$migration
SELECT 'FIRST'; $select
$migration
SELECT 'SECOND'; $select
SELECT 'END';
SELECT 'other-changes',COUNT(*) FROM saved_conditions a LEFT JOIN conditions b ON a.SourceTypeOrReferenceId=b.SourceTypeOrReferenceId AND a.SourceGroup=b.SourceGroup AND a.SourceEntry=b.SourceEntry AND a.SourceId=b.SourceId AND a.ElseGroup=b.ElseGroup AND a.ConditionTypeOrReference=b.ConditionTypeOrReference AND a.ConditionTarget=b.ConditionTarget AND a.ConditionValue1=b.ConditionValue1 AND a.ConditionValue2=b.ConditionValue2 AND a.ConditionValue3=b.ConditionValue3 AND a.NegativeCondition=b.NegativeCondition WHERE a.SourceEntry<>142487 AND b.SourceEntry IS NULL;
"@)
$a=[Array]::IndexOf($rows,'FIRST');$b=[Array]::IndexOf($rows,'SECOND');$e=[Array]::IndexOf($rows,'END')
if($a -lt 0 -or $b -le $a -or $e -le $b -or ($rows[($a+1)..($b-1)] -join "`n") -ne ($rows[($b+1)..($e-1)] -join "`n")){throw 'Lost Army migration is not stable on repeat'}
if($rows -notcontains "other-changes`t0" -or $b-$a -ne 8){throw 'Unexpected Lost Army condition scope'}
foreach($entry in @(115025,115031,115751,115846,116180,116600,116601)){
 $quest=if($entry -in @(115025,115031,115751)){44737}else{44923}
 if($rows -notcontains "$entry`t142487`t0`t0`t60`t0`t$quest`t0`t0`t0"){throw "Incorrect WQ condition for $entry"}
}
$rows | Set-Content "$out/replay.tsv"
$fixture=@(Query @"
CREATE TEMPORARY TABLE conditions AS SELECT * FROM conditions;
$migration
SELECT l.Entry,l.Item,l.Chance,l.QuestRequired,l.LootMode,l.GroupId,l.MinCount,l.MaxCount,COALESCE(c.ConditionValue1,0) FROM creature_loot_template l LEFT JOIN conditions c ON c.SourceTypeOrReferenceId=1 AND c.SourceGroup=l.Entry AND c.SourceEntry=l.Item AND c.ConditionTypeOrReference=60 WHERE l.Item=142487 ORDER BY l.Entry;
SELECT Entry,Item,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount,0 FROM gameobject_loot_template WHERE Item=142487;
"@);$fixture | Set-Content "$out/loot.tsv"
if($fixture.Count -ne 10){throw "Inspect fragment sources: count=$($fixture.Count), type=$($fixture.GetType().Name)"}
$world=@(Query @'
SELECT 'objective',COUNT(*) FROM quest_objectives WHERE QuestID=45088 AND Type=1 AND ObjectID=142487 AND Amount=1000 AND StorageIndex=0;
SELECT 'package',COUNT(*) FROM quest_template WHERE ID=45088 AND QuestPackageID=18559 AND RewardItem1=0 AND RewardChoiceItemID1=0;
SELECT 'starter',COUNT(*) FROM creature_queststarter WHERE quest=45088 AND id=114263;
SELECT 'enders',COUNT(*) FROM creature_questender WHERE quest=45088 AND id IN(114263,116760);
'@)
foreach($tag in @('objective','package','starter')){if($world -notcontains "$tag`t1"){throw "Lost Army data mismatch: $tag"}}
if($world -notcontains "enders`t2"){throw 'Lost Army turn-in relation mismatch'}
$world | Set-Content "$out/world.tsv"
$overlays=@(Query @'
SELECT COUNT(*) FROM quest_package_item WHERE PackageID=18559;
SELECT COUNT(*) FROM item_sparse WHERE ID IN(142487,143518,143514,143510,143506);
SELECT COUNT(*) FROM item_effect WHERE ItemID IN(143518,143514,143510,143506);
SELECT COUNT(*) FROM spell_effect WHERE SpellID IN(231683,231678,231674,231638);
'@ 'Hotfix')
if(($overlays -join ',') -ne '0,0,0,0'){throw 'Inspect Lost Army native data overlays'}
& "$PSScriptRoot/Test-LostArmyCatalog.ps1" *> "$out/catalog.log"
& "$PSScriptRoot/Test-StormheimHelheimSupplies.ps1" *> "$out/item-loot-base.log"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-helheim-supplies/*.inc" | Copy-Item -Destination $out
$base=(Get-Content "$out/HelheimSuppliesFixture.inc" -Raw).Replace('GetMaxStackSize()const{return 10;}','GetMaxStackSize()const{return 1000;}')
[IO.File]::WriteAllText("$out/LostArmyBaseFixture.inc",$base)
$f=Get-Content "$PSScriptRoot/StormheimHelheimSuppliesTest.cpp" -Raw;$f=$f.Substring(0,$f.IndexOf('int main('))
$f=$f.Replace('"HelheimSuppliesFixture.inc"','"LostArmyBaseFixture.inc"')
$f="using Condition=Gate::Condition;using ConditionList=std::vector<Condition*>;`n"+$f
$f=$f.Replace('struct LootStoreItem {','struct LootStoreItem {ConditionList conditions;')
$f=$f.Replace('struct LootItem {','struct LootItem {ConditionList conditions;bool AllowedForPlayer(Player const*)const;')
$f=$f.Replace('if(owner->HasQuestForItem(drop.itemid)){','if(owner->HasQuestForItem(drop.itemid)){LootItem visible;visible.conditions=drop.conditions;if(!visible.AllowedForPlayer(owner))continue;')
$f=$f.Replace('struct Group {void Process(', 'struct Group {LootStoreItemList explicitItems,equalItems;LootStoreItemList* GetExplicitlyChancedItemList(){return &explicitItems;}LootStoreItemList* GetEqualChancedItemList(){return &equalItems;}void Process(')
$f=$f.Replace('bool CheckItemCondition(Player const*','bool addConditionItem(Condition*);bool CheckItemCondition(Player const*')
$f+="`nstruct ConditionMgr {bool addToLootTemplate(Condition*,LootTemplate*);bool IsObjectMeetToConditions(Player*,ConditionList const& list){for(auto c:list)if(!c->Meets())return false;return true;}} conditionMgr;auto sConditionMgr=&conditionMgr;`n"
[IO.File]::WriteAllText("$out/LostArmyFixture.inc",$f)
function Export([string]$source,[string]$start,[string]$end,[string]$name){$a=$source.IndexOf($start);$b=$source.IndexOf($end,$a);if($a -lt 0 -or $b -le $a){throw "Missing native $name"};[IO.File]::WriteAllText("$out/$name.inc",$source.Substring($a,$b-$a))}
$conditions=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
Export $conditions '        case CONDITION_WORLD_QUEST:' '        case CONDITION_GAMEMASTER:' 'NativeWorldQuestCondition'
Export $conditions 'bool ConditionMgr::addToLootTemplate(' 'bool ConditionMgr::addToGossipMenus(' 'NativeConditionLootLoader'
$quests=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
Export $quests 'WorldQuest const* QuestDataStoreMgr::GetWorldQuest(' 'WorldQuest const* QuestDataStoreMgr::GenerateNewWorldQuest(' 'NativeWorldQuestLookup'
$loot=Get-Content "$repo/src/server/game/Loot/LootMgr.cpp" -Raw
Export $loot 'bool LootTemplate::addConditionItem(' 'bool LootTemplate::isReference(' 'NativeAttachLootCondition'
Export $loot 'bool LootItem::AllowedForPlayer(' 'void LootItem::AddAllowedLooter(' 'NativeLootItemCondition'
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/LostArmyLootTest.cpp`" /Fe`"$out/LostArmy.exe`" /Fo`"$out/LostArmy.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/LostArmy.exe`" `"$out/loot.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd";if($LASTEXITCODE){throw 'Lost Army native loot/quest regression failed'}
