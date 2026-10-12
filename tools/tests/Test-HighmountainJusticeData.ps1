param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_13_00_highmountain_justice.sql" -Raw
$sql=''
if(!$Live){
 foreach($table in @('creature_template','smart_scripts','spell_script_names','quest_template_addon')){
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $sql+="$migration`n$migration`n"
}
$sql+=@"
SELECT 'flight',COUNT(*) FROM creature_template WHERE entry=100711 AND VehicleId=4512 AND AIName='' AND ScriptName='npc_highmountain_justice' AND spell1=213467 AND spell2=198380 AND spell3=213444;
SELECT 'old-flight-events',COUNT(*) FROM smart_scripts WHERE entryorguid=100711 AND source_type=0 AND id BETWEEN 0 AND 3;
SELECT 'early-credit',COUNT(*) FROM smart_scripts WHERE entryorguid=100712 AND source_type=0 AND event_type=73 AND action_type=33 AND action_param1=100712;
SELECT 'spell-scripts',COUNT(*) FROM spell_script_names WHERE (spell_id=198214 AND ScriptName='spell_highmountain_justice') OR (spell_id=195024 AND ScriptName='spell_highmountain_justice_heal');
SELECT 'wounded',COUNT(*) FROM creature_template WHERE entry IN(97498,97506) AND AIName='' AND ScriptName='npc_highmountain_justice_wounded';
SELECT 'source',COUNT(*) FROM creature WHERE guid=14641574 AND id=100712 AND map=1220 AND PhaseId='5050' AND ABS(position_x-3931.5)<0.01 AND ABS(position_y-3929.8)<0.01 AND ABS(position_z-905.317)<0.01;
SELECT 'click',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=100712 AND spell_id=198214 AND cast_flags=1;
SELECT 'totem',COUNT(*) FROM creature_template_addon WHERE entry=97508 AND auras='195025';
SELECT 'strafe',COUNT(*) FROM areatrigger_actions WHERE entry=7105 AND moment=1 AND actionType=0 AND targetFlags=4096 AND spellId=213474;
SELECT 'rocklord-credit',COUNT(*) FROM creature_template_wdb WHERE Entry=97235 AND KillCredit1=97241;
SELECT 'objectives',COUNT(*) FROM quest_objectives WHERE QuestID=40594 AND ((ID=280942 AND ObjectID=100712 AND Amount=1 AND StorageIndex=0 AND Flags=1) OR (ID=284259 AND ObjectID=97241 AND Amount=100 AND TaskStep=1 AND Flags=92) OR (ID=280933 AND ObjectID=97217 AND Amount=20 AND TaskStep=5 AND Flags=92) OR (ID=284260 AND ObjectID=97498 AND Amount=20 AND TaskStep=5 AND Flags=92) OR (ID=284382 AND Type=15 AND Flags=2));
SELECT 'handoff',COUNT(*) FROM quest_template_addon WHERE ID=39780 AND PrevQuestID=40594;
SELECT 'lasan',COUNT(*) FROM creature WHERE guid=340650 AND id=97407 AND map=1220 AND PhaseId='' AND ABS(position_x-3926.54)<0.01 AND ABS(position_y-3924.1)<0.01;
SELECT 'starter',COUNT(*) FROM creature_queststarter WHERE id=97407 AND quest IN(40594,39780);
SELECT 'ender',COUNT(*) FROM creature_questender WHERE id=97407 AND quest=40594;
SELECT 'wounded-spawns',COUNT(*) FROM creature WHERE map=1220 AND id IN(97498,97506) AND PhaseId='5165';
SELECT 'boarding-phase',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7503 AND SourceEntry=86 AND ConditionTypeOrReference=9 AND ConditionValue1=40594 AND NegativeCondition=0;
"@
$expected=@("flight`t1","old-flight-events`t0","early-credit`t0","spell-scripts`t2","wounded`t2","source`t1","click`t1","totem`t1","strafe`t1","rocklord-credit`t1","objectives`t5","handoff`t1","lasan`t1","starter`t2","ender`t1","wounded-spawns`t35","boarding-phase`t1")
if(!$Live){
 $sql+=@"
SELECT 'other-ai',COUNT(*) FROM saved_creature_template a JOIN creature_template b ON a.entry=b.entry WHERE a.entry NOT IN(100711,97498,97506) AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName);
SELECT 'other-chain',COUNT(*) FROM saved_quest_template_addon a JOIN quest_template_addon b ON a.ID=b.ID WHERE a.ID<>39780 AND a.PrevQuestID<>b.PrevQuestID;
SELECT 'other-events',(SELECT COUNT(*) FROM saved_smart_scripts WHERE entryorguid NOT IN(100711,100712))-(SELECT COUNT(*) FROM smart_scripts WHERE entryorguid NOT IN(100711,100712));
UPDATE creature_template SET ScriptName='custom_justice' WHERE entry IN(100711,97498,97506);
UPDATE quest_template_addon SET PrevQuestID=123 WHERE ID=39780;
"@
 $sql+="$migration`nSELECT 'custom-ai',COUNT(*) FROM creature_template WHERE entry IN(100711,97498,97506) AND ScriptName='custom_justice'; SELECT 'custom-chain',COUNT(*) FROM quest_template_addon WHERE ID=39780 AND PrevQuestID=123;"
 $expected+=@("other-ai`t0","other-chain`t0","other-events`t0","custom-ai`t3","custom-chain`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names
 if($LASTEXITCODE){throw 'Highmountain Justice SQL test failed'}
} finally {$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Highmountain Justice SQL mismatch: $($actual -join '|')"}
'PASS: Justice migration, native objectives/abilities, boarding source, wounded actors, Lasan handoff and preserved custom data.'
