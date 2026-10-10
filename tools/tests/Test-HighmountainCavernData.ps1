param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_10_07_highmountain_cavern.sql" -Raw
$native=Get-Content "$repo/docs/audits/highmountain-cavern-native-2026-10-10.json" -Raw | ConvertFrom-Json
$nativeRows=($native.objectives | ForEach-Object {"SELECT $($_.ID) ID,$($_.QuestID) QuestID,$($_.Type) Type,$($_.StorageIndex) StorageIndex,$($_.ObjectID) ObjectID,$($_.Amount) Amount,$($_.Flags) Flags,$($_.Flags2) Flags2"}) -join ' UNION ALL '
$sql=''
if(!$Live){
 $sql=(@('quest_objectives','creature_template_wdb','quest_template_addon','creature_queststarter','gameobject_template','smart_scripts') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'native',COUNT(*) FROM ($nativeRows) a LEFT JOIN quest_objectives b USING(ID) WHERE b.ID IS NULL OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2;
SELECT 'crageater',KillCredit1,KillCredit2 FROM creature_template_wdb WHERE Entry=95916;
SELECT 'gelmogg-alias',KillCredit2 FROM creature_template_wdb WHERE Entry=95882;
SELECT 'parallel-pair',COUNT(*) FROM quest_template_addon WHERE ID IN(39488,39489) AND PrevQuestID=39661 AND NextQuestID=39487 AND ExclusiveGroup=-39488;
SELECT 'group-members',COUNT(*) FROM quest_template_addon WHERE ExclusiveGroup=-39488;
SELECT 'no-bypass',COUNT(*) FROM quest_template_addon WHERE NextQuestID=39487 AND ID NOT IN(39488,39489);
SELECT 'boss-prerequisite',PrevQuestID FROM quest_template_addon WHERE ID=39487;
SELECT 'boss-to-riverbend',COUNT(*) FROM quest_template_addon WHERE (ID=39498 AND PrevQuestID=39487 AND NextQuestID=42104) OR (ID=42104 AND PrevQuestID=39498);
SELECT 'jale-starters',COUNT(*) FROM creature_queststarter WHERE id=96520 AND quest IN(39488,39489,39487,39498);
SELECT 'native-enders',COUNT(*) FROM creature_questender WHERE (id=96520 AND quest IN(39661,39488,39489,39487)) OR (id=96038 AND quest IN(39488,39489,39487)) OR (id=97662 AND quest=39498);
SELECT 'creature-crystals',COUNT(*) FROM creature_loot_template WHERE Entry IN(95866,95916) AND Item=128393 AND Chance>0 AND QuestRequired=1 AND MinCount=1 AND MaxCount=1;
SELECT 'chest-loot-link',Data1 FROM gameobject_template WHERE entry=243639;
SELECT 'chest-loot',COUNT(*) FROM gameobject_loot_template WHERE Entry=243639 AND Item=128393 AND Chance=100 AND QuestRequired=1;
SELECT 'gelmogg-threshold',event_param1,event_param2,event_flags FROM smart_scripts WHERE entryorguid=95881 AND source_type=0 AND id=3 AND event_type=2 AND action_type=80 AND action_param1=9588100;
SELECT 'morph',action_param1 FROM smart_scripts WHERE entryorguid=9588100 AND source_type=9 AND id=5 AND action_type=36;
SELECT 'phase-signal',target_param1 FROM smart_scripts WHERE entryorguid=9946000 AND source_type=9 AND id=6 AND action_type=45 AND target_type=11;
SELECT 'phase-receiver',action_param1 FROM smart_scripts WHERE entryorguid=95881 AND source_type=0 AND id=4 AND event_type=38 AND event_param1=1 AND event_param2=1 AND action_type=22;
SELECT 'phase-spells',COUNT(*) FROM smart_scripts WHERE entryorguid=95881 AND source_type=0 AND id IN(5,6,7) AND event_phase_mask=2 AND action_type=11;
SELECT 'threshold-ai',COUNT(*) FROM creature_template WHERE entry IN(95881,95882) AND AIName='SmartAI';
SELECT 'level-scaling',COUNT(*) FROM creature_template_scaling WHERE Entry IN(95881,95882,95866,95916) AND LevelScalingMin=98 AND LevelScalingMax=110;
SELECT 'spawns',(SELECT COUNT(*)>=7 FROM creature WHERE id IN(95866,95916) AND map=1220),(SELECT COUNT(*) FROM creature WHERE id=95881 AND map=1220),(SELECT COUNT(*) FROM creature WHERE guid IN(340583,340588) AND map=1220);
SELECT 'delivery-objectives',COUNT(*) FROM quest_objectives WHERE QuestID IN(39661,39498);
SELECT 'no-instant-script',COUNT(*) FROM quest_start_scripts WHERE id IN(39661,39488,39489,39487,39498);
SELECT 'reward-package',QuestPackageID FROM quest_template WHERE ID=39487;
SELECT 'native-next-quest',RewardNextQuest FROM quest_template WHERE ID=39277;
"@
$expected=@("native`t0","crageater`t95866`t0","gelmogg-alias`t95881","parallel-pair`t2","group-members`t2","no-bypass`t0","boss-prerequisite`t39489","boss-to-riverbend`t2","jale-starters`t4","native-enders`t8","creature-crystals`t2","chest-loot-link`t243639","chest-loot`t1","gelmogg-threshold`t0`t50`t1","morph`t95882","phase-signal`t95882","phase-receiver`t2","phase-spells`t3","threshold-ai`t2","level-scaling`t4","spawns`t1`t1`t2","delivery-objectives`t0","no-instant-script`t0","reward-package`t9699","native-next-quest`t39487")
if(!$Live){
 $sql+=@"
SELECT 'unrelated-smart',COUNT(*) FROM smart_scripts a WHERE a.entryorguid NOT IN(95881,9946000) AND NOT EXISTS(SELECT 1 FROM saved_smart_scripts b WHERE a.entryorguid=b.entryorguid AND a.source_type=b.source_type AND a.id=b.id AND a.action_type=b.action_type AND a.action_param1=b.action_param1 AND a.target_type=b.target_type AND a.target_param1=b.target_param1 AND a.link=b.link);
SELECT 'unrelated-objectives',COUNT(*) FROM quest_objectives a JOIN saved_quest_objectives b USING(ID) WHERE a.ID<>279636 AND (a.Flags2<>b.Flags2 OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount);
SELECT 'unrelated-credits',COUNT(*) FROM creature_template_wdb a JOIN saved_creature_template_wdb b USING(Entry) WHERE a.Entry<>95916 AND (a.KillCredit1<>b.KillCredit1 OR a.KillCredit2<>b.KillCredit2);
"@
 $expected+=@("unrelated-smart`t0","unrelated-objectives`t0","unrelated-credits`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Cavern SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($rows -join '|') -ne ($expected -join '|')){throw "Cavern SQL mismatch: $($rows -join '|')"}
'PASS: native objectives/alternate credit, parallel prerequisite group without bypass, Jale relations, both crystal drop sources, chest loot binding, transformed boss phase signal and unchanged unrelated records.'
