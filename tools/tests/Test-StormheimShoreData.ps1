param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_13_16_stormheim_shore.sql" -Raw
$sql=''
if(!$Live){
 foreach($table in @('gameobject_template','quest_objectives','spell_script_names')){
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $sql+="UPDATE gameobject_template SET Data1=0 WHERE entry=241275; UPDATE gameobject_template SET ScriptName='',Data10=181981 WHERE entry IN(240600,249829); UPDATE quest_objectives SET Flags=0 WHERE ID=277818;`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'medical-link',COUNT(*) FROM gameobject_template WHERE entry=241275 AND type=3 AND Data1=241275 AND Data0=1691;
SELECT 'medical-loot',COUNT(*) FROM gameobject_loot_template WHERE Entry=241275 AND Item=123876 AND Reference=0 AND Chance=100 AND QuestRequired=1 AND LootMode=1 AND MinCount=1 AND MaxCount=1;
SELECT 'battery-loot',COUNT(*) FROM creature_loot_template WHERE Entry=90902 AND Item=122295 AND Reference=0 AND Chance>0 AND QuestRequired=1 AND LootMode=0 AND GroupId=0 AND MinCount=1 AND MaxCount=1;
SELECT 'waste-objectives',COUNT(*) FROM quest_objectives WHERE (ID=277818 AND QuestID=38358 AND Type=0 AND StorageIndex=1 AND ObjectID=91693 AND Amount=100 AND (Flags & 12)=12) OR(ID=277817 AND QuestID=38358 AND Type=0 AND StorageIndex=0 AND ObjectID=91590 AND Amount=1 AND Flags=0);
SELECT 'barrels',COUNT(*) FROM gameobject_template WHERE entry IN(240600,249829) AND type=10 AND Data0=2382 AND Data3=1000 AND Data5=0 AND Data10=0 AND Data23=1 AND ScriptName='go_stormheim_volatile_barrel';
SELECT 'pump-script',COUNT(*) FROM spell_script_names WHERE spell_id=181981 AND ScriptName='spell_stormheim_shore_pump';
SELECT 'seven-prerequisites',COUNT(*) FROM quest_template_addon WHERE (ID IN(38036,38052,38053,38057) AND PrevQuestID=39800) OR(ID IN(38332,38357,38358) AND PrevQuestID=39801);
SELECT 'seven-event-locks',COUNT(*) FROM quest_template_addon WHERE ID IN(38036,38052,38053,38057,38332,38357,38358) AND (SpecialFlags & 2)<>0;
SELECT 'native-report-quests',COUNT(*) FROM quest_template WHERE ID IN(38057,38332) AND QuestType=2;
SELECT 'report-objectives',COUNT(*) FROM quest_objectives WHERE QuestID IN(38057,38332);
SELECT 'quest-start-shortcuts',COUNT(*) FROM quest_start_scripts WHERE id IN(38036,38052,38053,38057,38332,38357,38358) AND command=7;
SELECT 'seven-starters',COUNT(*) FROM creature_queststarter WHERE (id=90749 AND quest=38052) OR(id=90783 AND quest IN(38036,38057)) OR(id=90866 AND quest=38053) OR(id=91473 AND quest=38332) OR(id=91590 AND quest IN(38357,38358));
SELECT 'seven-enders',COUNT(*) FROM creature_questender WHERE (id=90749 AND quest=38052) OR(id=90783 AND quest=38036) OR(id=90866 AND quest=38053) OR(id=92414 AND quest=38057) OR(id=91158 AND quest=38332) OR(id=91590 AND quest IN(38357,38358)) OR(id=91693 AND quest=38358);
SELECT 'fleet-hand-in',COUNT(*) FROM creature_questender WHERE (id=90749 AND quest=39800) OR(id=91473 AND quest=39801);
SELECT 'kill-aliases',COUNT(*) FROM creature_template_wdb WHERE (Entry=92224 AND KillCredit1=90899) OR(Entry=91563 AND KillCredit1=91569);
SELECT 'combat-scaling',COUNT(*) FROM creature_template_scaling WHERE Entry IN(90899,92224,90902,91563,91569) AND LevelScalingMin=98 AND LevelScalingMax=110;
SELECT 'medical-spawns',COUNT(*) FROM gameobject WHERE id=241275 AND map=1220 AND phaseMask=1;
SELECT 'barrel-spawns',COUNT(*) FROM gameobject WHERE id IN(240600,249829) AND map=1220 AND areaId=7563 AND phaseMask=1;
SELECT 'questgiver-flags',COUNT(*) FROM creature_template WHERE entry IN(90749,90783,90866,91473,91590,91693,91158,92414) AND (npcflag & 2)<>0;
SELECT 'next-links',COUNT(*) FROM quest_template_addon WHERE (ID=38053 AND NextQuestID=38058) OR(ID=38057 AND NextQuestID=38059) OR(ID=38332 AND NextQuestID=38360);
"@
$expected=@("medical-link`t1","medical-loot`t1","battery-loot`t1","waste-objectives`t2","barrels`t2","pump-script`t1","seven-prerequisites`t7","seven-event-locks`t0","native-report-quests`t2","report-objectives`t0","quest-start-shortcuts`t0","seven-starters`t7","seven-enders`t8","fleet-hand-in`t2","kill-aliases`t2","combat-scaling`t5","medical-spawns`t9","barrel-spawns`t21","questgiver-flags`t8","next-links`t3")
if(!$Live){
 $sql+=@"
SELECT 'other-objects',COUNT(*) FROM gameobject_template a JOIN saved_gameobject_template b USING(entry) WHERE a.entry NOT IN(241275,240600,249829) AND (a.Data1<>b.Data1 OR a.Data10<>b.Data10 OR a.ScriptName<>b.ScriptName);
SELECT 'other-objectives',COUNT(*) FROM quest_objectives a JOIN saved_quest_objectives b USING(ID) WHERE a.ID<>277818 AND a.Flags<>b.Flags;
UPDATE gameobject_template SET ScriptName='custom',Data10=181981 WHERE entry=240600;
UPDATE gameobject_template SET Data1=777 WHERE entry=241275;
UPDATE quest_objectives SET Flags=Flags | 1 WHERE ID=277818;
"@
 $sql+="$migration`nSELECT 'custom-barrel',COUNT(*) FROM gameobject_template WHERE entry=240600 AND ScriptName='custom' AND Data10=181981; SELECT 'custom-chest',COUNT(*) FROM gameobject_template WHERE entry=241275 AND Data1=777; SELECT 'additional-flags',COUNT(*) FROM quest_objectives WHERE ID=277818 AND Flags=13;"
 $expected+=@("other-objects`t0","other-objectives`t0","custom-barrel`t1","custom-chest`t1","additional-flags`t1")
}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names
 if($LASTEXITCODE){throw 'Shore SQL test failed'}
} finally {$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Shore SQL mismatch: $($actual -join '|')"}
'PASS: seven quest gates and giver/ender/next graph, medical loot link, unchanged valid battery loot, kill aliases/scaling, native report quests, barrel lifecycle and migration idempotence/custom preservation.'
