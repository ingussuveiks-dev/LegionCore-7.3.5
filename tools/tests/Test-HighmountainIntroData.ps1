param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_10_05_highmountain_intro.sql" -Raw
$native=Get-Content "$repo/docs/audits/highmountain-intro-native-2026-10-10.json" -Raw | ConvertFrom-Json
$nativeRows=($native.objectives | ForEach-Object {"SELECT $($_.ID) ID,$($_.QuestID) QuestID,$($_.Type) Type,$($_.StorageIndex) StorageIndex,$($_.ObjectID) ObjectID,$($_.Amount) Amount,$($_.Flags) Flags,$($_.Flags2) Flags2"}) -join ' UNION ALL '
$tables=@('quest_objectives','quest_start_scripts','conditions','smart_scripts','spell_script_names','creature_template','creature_questender','creature','quest_template','quest_poi')
$sql=''
if(!$Live){
 $sql=($tables | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'native',COUNT(*) FROM ($nativeRows) a LEFT JOIN quest_objectives b USING(ID) WHERE b.ID IS NULL OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2;
SELECT 'instant-completion',COUNT(*) FROM quest_start_scripts WHERE id=39733 AND command=7 AND datalong=39733;
SELECT 'event-gate',SpecialFlags&2 FROM quest_template_addon WHERE ID=39733;
SELECT 'oro-dalaran',COUNT(*) FROM smart_scripts WHERE entryorguid=97666 AND source_type=0 AND id=3 AND action_type=15 AND action_param1=39733;
SELECT 'alive-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=4 AND SourceEntry=97666 AND ConditionTypeOrReference=36;
SELECT 'no-premature-flight',COUNT(*) FROM smart_scripts WHERE entryorguid=97666 AND source_type=0 AND action_type=33 AND action_param1=96813;
SELECT 'discovery-link',COUNT(*) FROM smart_scripts WHERE entryorguid=97666 AND source_type=0 AND id=1 AND link=2 AND action_type=72;
SELECT 'discovery',COUNT(*) FROM smart_scripts WHERE entryorguid=97666 AND source_type=0 AND id=2 AND action_type=85 AND action_param1=198160;
SELECT 'flight-script',COUNT(*) FROM spell_script_names WHERE spell_id=198160 AND ScriptName='spell_highmountain_discover_thunder_totem';
SELECT 'npc-scripts',COUNT(*) FROM creature_template WHERE (entry=106244 AND ScriptName='npc_highmountain_arrival_oro') OR (entry IN(99433,99434,99435,99436) AND ScriptName='npc_highmountain_poison_idol');
SELECT 'old-idol-script',COUNT(*) FROM smart_scripts WHERE (entryorguid IN(99433,99434,99435,99436) AND source_type=0) OR (entryorguid IN(9943300,10624400) AND source_type=9);
SELECT 'distinct-idols',COUNT(*),COUNT(DISTINCT id) FROM creature WHERE guid IN(340566,340567,340568,340569) AND id IN(99433,99434,99435,99436) AND map=1220;
SELECT 'native-idol-locations',COUNT(*) FROM creature c JOIN quest_poi p ON p.QuestID=39272 AND p.QuestObjectID=c.id JOIN quest_poi_points pp ON pp.QuestID=p.QuestID AND pp.Idx1=p.Idx1 WHERE c.guid IN(340566,340567,340568,340569) AND ABS(c.position_x-pp.X)<3 AND ABS(c.position_y-pp.Y)<3;
SELECT 'clicks',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry IN(99433,99434,99435,99436) AND spell_id=195481 AND cast_flags=1;
SELECT 'enders',COUNT(*) FROM creature_questender WHERE quest=38907 AND id=93826;
SELECT 'wrong-ender',COUNT(*) FROM creature_questender WHERE quest=38907 AND id=93805;
SELECT 'bad-poi',COUNT(*) FROM quest_poi WHERE QuestID=38907 AND MapID=402653184;
SELECT 'valid-poi',COUNT(*) FROM quest_poi WHERE QuestID=38907 AND BlobIndex=2 AND Idx1=5 AND MapID=1220;
SELECT 'crop-reward',QuestPackageID FROM quest_template WHERE ID=39272;
SELECT 'ormgul-reward',QuestPackageID FROM quest_template WHERE ID=39491;
SELECT 'ormgul-loot',COUNT(*) FROM creature_loot_template WHERE Entry=95935 AND Item=128397 AND QuestRequired=1 AND LootMode=0 AND MinCount=1 AND MaxCount=1;
SELECT 'links',COUNT(*) FROM quest_template_addon WHERE (ID=38907 AND PrevQuestID=39733) OR (ID=38911 AND PrevQuestID=38907) OR (ID IN(39272,39490,39491) AND PrevQuestID=38911) OR (ID=39496 AND PrevQuestID=39491);
SELECT 'quest-starters',COUNT(*) FROM creature_queststarter WHERE (id=97666 AND quest=38907) OR (id=93826 AND quest=38911) OR (id=93833 AND quest IN(39491,39496)) OR (id=95191 AND quest IN(39272,39490));
SELECT 'farm-spawns',COUNT(DISTINCT id) FROM creature WHERE map=1220 AND id IN(93833,95191,95186,95935,94688,94691);
"@
$expected=@("native`t0","instant-completion`t0","event-gate`t2","oro-dalaran`t1","alive-condition`t1","no-premature-flight`t0","discovery-link`t1","discovery`t1","flight-script`t1","npc-scripts`t5","old-idol-script`t0","distinct-idols`t4`t4","native-idol-locations`t4","clicks`t4","enders`t1","wrong-ender`t0","bad-poi`t0","valid-poi`t1","crop-reward`t0","ormgul-reward`t679","ormgul-loot`t1","links`t6","quest-starters`t6","farm-spawns`t6")
if(!$Live){
 $sql+=@"
SELECT 'unrelated-smart',COUNT(*) FROM smart_scripts a WHERE a.entryorguid NOT IN(97666,106244,10624400,99433,99434,99435,99436,9943300) AND NOT EXISTS(SELECT 1 FROM saved_smart_scripts b WHERE a.entryorguid=b.entryorguid AND a.source_type=b.source_type AND a.id=b.id AND a.action_type=b.action_type AND a.action_param1=b.action_param1 AND a.link=b.link);
SELECT 'aludane-preserved',(SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=96813)-(SELECT COUNT(*) FROM saved_smart_scripts WHERE entryorguid=96813);
SELECT 'unrelated-spawns',COUNT(*) FROM creature a JOIN saved_creature b USING(guid) WHERE a.guid NOT IN(340567,340568,340569) AND a.id<>b.id;
UPDATE creature_template SET ScriptName='custom_idol',AIName='custom_ai' WHERE entry=99434;
$migration
SELECT 'custom',ScriptName,AIName FROM creature_template WHERE entry=99434;
"@
 $expected+=@("unrelated-smart`t0","aludane-preserved`t0","unrelated-spawns`t0","custom`tcustom_idol`tcustom_ai")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Highmountain SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($rows -join '|') -ne ($expected -join '|')){throw "Highmountain SQL mismatch: $($rows -join '|')"}
if($Live){'PASS: installed Highmountain intro objectives, real-action bindings, four distinct native POI idols, chain, rewards, loot and spawn records.'}
else{'PASS: migration twice on temporary table copies, 11 native objectives, actual action bindings, four POI-matched idols, chain/rewards, retained Ormgul loot and Aludane/other custom scripts.'}
