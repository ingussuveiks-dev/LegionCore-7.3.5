param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_15_35_stormheim_runewood.sql" -Raw
$sql=''
if(!$Live){
 $sql="CREATE TEMPORARY TABLE creature_template AS SELECT * FROM creature_template WHERE entry IN(93182,93342,93343); CREATE TEMPORARY TABLE spell_target_position AS SELECT * FROM spell_target_position; CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM spell_script_names; DELETE FROM spell_script_names WHERE ScriptName IN('spell_stormheim_runewood_turn','spell_stormheim_runewood_summon'); DELETE FROM spell_target_position WHERE id IN(184582,184792,184794); UPDATE creature_template SET AIName='SmartAI',ScriptName='' WHERE entry IN(93182,93342,93343);`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'actors',COUNT(*) FROM creature_template WHERE entry IN(93182,93342,93343) AND AIName='' AND ScriptName='npc_stormheim_runewood_stone' AND npcflag=16777216;
SELECT 'destinations',COUNT(*) FROM spell_target_position WHERE (id=184582 AND target_map=1220 AND ABS(target_position_x-3116.57)<0.01 AND ABS(target_position_y-1247.91)<0.01 AND ABS(target_position_z-125.037)<0.01) OR(id=184792 AND target_map=1220 AND ABS(target_position_x-2992.34)<0.01 AND ABS(target_position_y-1126.41)<0.01 AND ABS(target_position_z-130.259)<0.01) OR(id=184794 AND target_map=1220 AND ABS(target_position_x-3181.52)<0.01 AND ABS(target_position_y-1054.34)<0.01 AND ABS(target_position_z-117.037)<0.01);
SELECT 'bindings',COUNT(*) FROM spell_script_names WHERE (spell_id=184579 AND ScriptName='spell_stormheim_runewood_turn') OR(spell_id IN(184582,184792,184794) AND ScriptName='spell_stormheim_runewood_summon');
SELECT 'clicks',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry IN(93182,93342,93343) AND spell_id=184579 AND cast_flags=1;
SELECT 'original-sai',COUNT(*) FROM smart_scripts WHERE (source_type=0 AND entryorguid IN(93182,93342,93343)) OR(source_type=9 AND entryorguid IN(9318200,9334200,9334300));
SELECT 'travel-empty',COUNT(*) FROM quest_objectives WHERE QuestID=39796;
SELECT 'objectives',COUNT(*) FROM quest_objectives WHERE QuestID=38778 AND ((ObjectID=93196 AND Amount=3 AND StorageIndex=0 AND Flags=0) OR(ObjectID=93185 AND Amount=1 AND StorageIndex=1 AND Flags=28) OR(ObjectID=93340 AND Amount=1 AND StorageIndex=2 AND Flags=28) OR(ObjectID=93341 AND Amount=1 AND StorageIndex=3 AND Flags=28));
SELECT 'chain',COUNT(*) FROM quest_template_addon WHERE (ID=39796 AND PrevQuestID=39804 AND NextQuestID=38778) OR(ID=38778 AND PrevQuestID=39796 AND NextQuestID=38810);
SELECT 'native-quests',COUNT(*) FROM quest_template WHERE (ID=39796 AND Flags=37748744 AND FlagsEx=8192 AND StartScript=0 AND RewardNextQuest=38778) OR(ID=38778 AND Flags=39845896 AND FlagsEx=270336 AND StartScript=0 AND RewardNextQuest=38810);
SELECT 'vydhar',COUNT(*) FROM creature WHERE guid=292106 AND id=93231 AND map=1220 AND zoneId=7541 AND ABS(position_x-3073.09)<0.01 AND ABS(position_y-1056.88)<0.01;
SELECT 'relations',COUNT(*) FROM creature_questender WHERE id=93231 AND quest IN(39796,38778);
SELECT 'next-relations',COUNT(*) FROM creature_queststarter WHERE id=93231 AND quest IN(38778,38810);
SELECT 'base-anchors',COUNT(*) FROM gameobject WHERE id=241763 AND guid IN(115106,115223,115221) AND map=1220;
SELECT 'model',COUNT(*) FROM creature_template_wdb WHERE Entry IN(93182,93342,93343) AND Displayid1=63471 AND CursorName='questinteract';
SELECT 'no-click-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup IN(93182,93342,93343);
SELECT 'no-implicit-entry-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry IN(184582,184792,184794);
"@
$expected=@("actors`t3","destinations`t3","bindings`t4","clicks`t3","original-sai`t12","travel-empty`t0","objectives`t4","chain`t2","native-quests`t2","vydhar`t1","relations`t2","next-relations`t2","base-anchors`t3","model`t3","no-click-conditions`t0","no-implicit-entry-conditions`t0")
if(!$Live){
 $reset="UPDATE creature_template SET AIName='SmartAI',ScriptName='' WHERE entry IN(93182,93342,93343); DELETE FROM spell_target_position WHERE id IN(184582,184792,184794); DELETE FROM spell_script_names WHERE ScriptName IN('spell_stormheim_runewood_turn','spell_stormheim_runewood_summon');"
 $sql+="$reset UPDATE creature_template SET ScriptName='custom_rune' WHERE entry=93182;`n$migration`nSELECT 'custom-actor',COUNT(*) FROM creature_template WHERE entry=93182 AND ScriptName='custom_rune'; SELECT 'custom-no-bindings',COUNT(*) FROM spell_script_names WHERE ScriptName IN('spell_stormheim_runewood_turn','spell_stormheim_runewood_summon'); $reset INSERT INTO spell_target_position VALUES(184582,1220,1,2,3,0);`n$migration`nSELECT 'custom-dest',COUNT(*) FROM spell_target_position WHERE id=184582 AND target_position_x=1; SELECT 'custom-no-merge',COUNT(*) FROM spell_target_position WHERE id IN(184582,184792,184794);"
 $expected+=@("custom-actor`t1","custom-no-bindings`t0","custom-dest`t1","custom-no-merge`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=$sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Runewood SQL failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Runewood SQL mismatch: $($actual -join '|')"}
if($Live){'PASS: installed Runewood personal actor bindings, native destinations/clicks/markers/travel/chain/anchors; original SAI retained.'}else{'PASS: Runewood migration twice, native quest/actor/SAI data and custom actors/destinations preserved.'}
