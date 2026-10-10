$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_10_00_eye_quest_chain.sql" -Raw
$tables=@('smart_scripts','gameobject_template','quest_template')
$fixture=($tables|ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$native=Get-Content "$repo/docs/audits/eye-quest-chain-native-2026-10-10.json" -Raw|ConvertFrom-Json
$nativeRows=($native.objectives|ForEach-Object {"SELECT $($_.ID) ID,$($_.QuestID) QuestID,$($_.Type) Type,$($_.StorageIndex) StorageIndex,$($_.ObjectID) ObjectID,$($_.Amount) Amount,$($_.Flags) Flags,$($_.Flags2) Flags2"}) -join ' UNION ALL '
$fixture+=@"
$migration
$migration
SELECT 'native-objectives',COUNT(*) FROM ($nativeRows) a LEFT JOIN quest_objectives b USING(ID) WHERE b.ID IS NULL OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2;
SELECT 'old-proximity',COUNT(*) FROM smart_scripts WHERE (entryorguid=106847 AND action_param1=106847) OR (entryorguid=106815 AND action_param1=106815);
SELECT 'unrelated-credit',COUNT(*) FROM smart_scripts WHERE entryorguid=106815 AND id=1 AND action_param1=109750;
SELECT 'pads',COUNT(*) FROM gameobject_template WHERE entry IN(244534,244560) AND type=5 AND ScriptName='go_eye_portrait_teleporter';
SELECT 'dungeon-reward',QuestPackageID,RewardItem1,RewardAmount1 FROM quest_template WHERE ID=38286;
SELECT 'placement-item',QuestPackageID,StartItem,ItemDrop1,ItemDropQuantity1 FROM quest_template WHERE ID=42213;
SELECT 'links',COUNT(*) FROM quest_template_addon WHERE (ID=38286 AND PrevQuestID=37470 AND NextQuestID=42213) OR (ID=42213 AND PrevQuestID=38286 AND ProvidedItemCount=1 AND SpecialFlags=0);
SELECT 'questender',COUNT(*) FROM gameobject_questender WHERE id=246465 AND quest=42213;
SELECT 'before-placement',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7502 AND SourceEntry=222 AND ConditionTypeOrReference=8 AND ConditionValue1=42213 AND NegativeCondition=1;
SELECT 'boss-scripts',COUNT(*) FROM creature_template WHERE entry IN(91784,91789,91797,91808,96028) AND ScriptName<>'';
SELECT 'spawns',COUNT(*) FROM creature WHERE map=1456 AND id IN(91784,91789,91797,91808,96028,106780) AND spawnMask=8388870;
SELECT 'unrelated-templates',COUNT(*) FROM gameobject_template a JOIN saved_gameobject_template b USING(entry) WHERE a.entry NOT IN(244534,244560) AND a.ScriptName<>b.ScriptName;
UPDATE gameobject_template SET ScriptName='custom_pad' WHERE entry=244534;
$migration
SELECT 'custom',ScriptName FROM gameobject_template WHERE entry=244534;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$fixture;if($LASTEXITCODE){throw 'Eye SQL fixture failed'}}finally{$env:MYSQL_PWD=$old}
$expected=@("native-objectives`t0","old-proximity`t0","unrelated-credit`t1","pads`t2","dungeon-reward`t0`t141385`t1","placement-item`t18462`t137206`t137206`t1","links`t2","questender`t1","before-placement`t1","boss-scripts`t5","spawns`t6","unrelated-templates`t0","custom`tcustom_pad")
if(($rows -join '|') -ne ($expected -join '|')){throw "SQL mismatch: $($rows -join '|')"}
'PASS: actual migration twice in temporary tables; native objective storage, correct rewards, item provision/removal, chain links, turn-in phase, all boss bindings/spawns, unrelated credit/custom scripts retained.'
