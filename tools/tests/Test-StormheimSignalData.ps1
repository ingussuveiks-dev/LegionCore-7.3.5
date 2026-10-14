param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_14_03_stormheim_signal.sql" -Raw
$sql=''
if(!$Live){
 foreach($table in @('creature_template','gameobject_template','gameobject')){
  $filter=switch($table){
   'creature_template' {' WHERE entry IN(90899,90907,92475,92547,92556,92557,91571)'}
   'gameobject_template' {' WHERE entry IN(240235,241031)'}
   'gameobject' {' WHERE id=240235 OR guid=114735'}
  }
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table$filter; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 # MySQL cannot self-join a temporary table. Only the read-only original
 # alias is redirected to the identical original snapshot for this test.
 $temporaryMigration=$migration.Replace('JOIN gameobject original','JOIN saved_gameobject original')
 $sql+="CREATE TEMPORARY TABLE duplicate_seed LIKE gameobject; INSERT INTO duplicate_seed SELECT * FROM saved_gameobject WHERE guid=127609; UPDATE duplicate_seed SET guid=114500,position_z=position_z+10; INSERT INTO gameobject SELECT * FROM duplicate_seed ON DUPLICATE KEY UPDATE position_z=VALUES(position_z);`n$temporaryMigration`n$temporaryMigration`n"
}
$sql+=@"
SELECT 'go-binding',COUNT(*) FROM gameobject_template WHERE entry=240235 AND ScriptName='go_stormheim_signal' AND type=10 AND Data0=1690 AND Data1=38060 AND Data2=44654 AND Data10=0;
SELECT 'controller-binding',COUNT(*) FROM creature_template WHERE entry=92475 AND ScriptName='npc_stormheim_signal_controller';
SELECT 'actor-bindings',COUNT(*) FROM creature_template WHERE entry IN(90899,90907,92547,92556,92557) AND ScriptName='npc_stormheim_signal_actor' AND AIName='SmartAI';
SELECT 'ground-propeller',COUNT(*) FROM gameobject WHERE guid=127609 AND id=240235 AND map=1220 AND ABS(position_z-526.777)<0.01;
SELECT 'elevated-duplicate',COUNT(*) FROM gameobject WHERE guid=114500 AND id=240235;
SELECT 'native-objectives',COUNT(*) FROM quest_objectives WHERE (ID=278188 AND QuestID=38060 AND Type=0 AND StorageIndex=1 AND ObjectID=92475 AND Amount=1 AND Flags=0) OR(ID=277473 AND QuestID=38060 AND Type=0 AND StorageIndex=0 AND ObjectID=90907 AND Amount=1 AND Flags=3) OR(ID=277873 AND QuestID=38362 AND Type=1 AND ObjectID=122746 AND Amount=1 AND Flags=0);
SELECT 'attackable-boss',COUNT(*) FROM creature_template WHERE entry=90907 AND faction=16 AND (unit_flags & 33555202)=0;
SELECT 'valid-scaling',COUNT(*) FROM creature_template_scaling WHERE Entry IN(90899,90907,91571) AND LevelScalingMin=98 AND LevelScalingMax=110;
SELECT 'native-arrival-path',COUNT(*) FROM waypoints WHERE entry=92547 AND pointid BETWEEN 1 AND 17;
SELECT 'trophy-mob',COUNT(*) FROM creature_template WHERE entry=91571 AND lootid=91571 AND faction=84;
SELECT 'trophy-spawn',COUNT(*) FROM creature WHERE guid=289127 AND id=91571 AND map=1220 AND zoneId=7541 AND PhaseId='';
SELECT 'native-head-loot',COUNT(*) FROM creature_loot_template WHERE Entry=91571 AND Item=122746 AND QuestRequired=1 AND Chance>98 AND Chance<=100 AND LootMode=0 AND GroupId=0 AND MinCount=1 AND MaxCount=1;
SELECT 'trophy-starter',COUNT(*) FROM creature_queststarter WHERE id=91158 AND quest=38362;
SELECT 'trophy-ender',COUNT(*) FROM creature_questender WHERE id=91158 AND quest=38362;
SELECT 'signal-enders',COUNT(*) FROM creature_questender WHERE id IN(90749,92557) AND quest=38060;
SELECT 'prerequisites',COUNT(*) FROM quest_template_addon WHERE (ID=38060 AND PrevQuestID=38059) OR(ID=38362 AND PrevQuestID=38360);
SELECT 'next-trials',COUNT(*) FROM quest_template_addon WHERE (ID=38210 AND PrevQuestID=38060) OR(ID=38459 AND PrevQuestID=38362);
SELECT 'next-trial-starters',COUNT(*) FROM creature_queststarter WHERE (id=97306 AND quest=38210) OR(id=97305 AND quest=38459);
"@
$expected=@("go-binding`t1","controller-binding`t1","actor-bindings`t5","ground-propeller`t1","elevated-duplicate`t0","native-objectives`t3","attackable-boss`t1","valid-scaling`t3","native-arrival-path`t17","trophy-mob`t1","trophy-spawn`t1","native-head-loot`t1","trophy-starter`t1","trophy-ender`t1","signal-enders`t2","prerequisites`t2","next-trials`t2","next-trial-starters`t2")
if(!$Live){
 $sql+=@"
SELECT 'other-template',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.entry=91571 AND (a.ScriptName<>b.ScriptName OR a.lootid<>b.lootid OR a.unit_flags<>b.unit_flags);
SELECT 'preserved-native-ai',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.AIName<>b.AIName OR a.unit_flags<>b.unit_flags;
SELECT 'other-go',COUNT(*) FROM gameobject_template a JOIN saved_gameobject_template b USING(entry) WHERE a.entry=241031 AND a.ScriptName<>b.ScriptName;
SELECT 'other-spawn',COUNT(*) FROM gameobject WHERE guid=114735;
UPDATE creature_template SET ScriptName='custom' WHERE entry=90907;
UPDATE gameobject_template SET ScriptName='custom' WHERE entry=240235;
INSERT INTO gameobject SELECT * FROM duplicate_seed;
UPDATE gameobject SET PhaseId='999' WHERE guid=114500;
"@
 $sql+="$temporaryMigration`nSELECT 'custom-actor',COUNT(*) FROM creature_template WHERE entry=90907 AND ScriptName='custom'; SELECT 'custom-object',COUNT(*) FROM gameobject_template WHERE entry=240235 AND ScriptName='custom'; SELECT 'custom-spawn',COUNT(*) FROM gameobject WHERE guid=114500 AND PhaseId='999';"
 $expected+=@("other-template`t0","preserved-native-ai`t0","other-go`t0","other-spawn`t1","custom-actor`t1","custom-object`t1","custom-spawn`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Signal SQL failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Signal SQL mismatch: $($actual -join '|')"}
'PASS: migration twice, propeller/controller/actor bindings, native sequenced objectives, attackable/scaled boss, flight path, actual trophy spawn/loot/hand-in/next trials and guarded custom/unrelated preservation.'
