param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_15_34_stormheim_bones.sql" -Raw
$sql=''
if(!$Live){
 $sql="CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM spell_script_names; CREATE TEMPORARY TABLE creature_template AS SELECT * FROM creature_template WHERE entry=98230; CREATE TEMPORARY TABLE conversation_actor AS SELECT * FROM conversation_actor WHERE entry=2247; UPDATE creature_template SET SpellStateAnimKitID=192171 WHERE entry=98230; UPDATE conversation_actor SET duration=3350 WHERE entry=2247 AND id=0; DELETE FROM spell_script_names WHERE spell_id=192171 AND ScriptName='spell_stormheim_bones_butchering';`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'binding',COUNT(*) FROM spell_script_names WHERE spell_id=192171 AND ScriptName='spell_stormheim_bones_butchering';
SELECT 'travel-empty',COUNT(*) FROM quest_objectives WHERE QuestID=39803;
SELECT 'bones-objective',COUNT(*) FROM quest_objectives WHERE QuestID=39804 AND ID=279981 AND Type=0 AND ObjectID=97340 AND Amount=1 AND StorageIndex=0 AND Flags=0;
SELECT 'chain',COUNT(*) FROM quest_template_addon WHERE (ID=39803 AND PrevQuestID=38624 AND NextQuestID=39804) OR(ID=39804 AND PrevQuestID=39803 AND NextQuestID=39796);
SELECT 'quest-definitions',COUNT(*) FROM quest_template WHERE (ID=39803 AND StartScript=0 AND Flags=38797320 AND FlagsEx=8192 AND RewardNextQuest=39804) OR(ID=39804 AND StartScript=0 AND Flags=37748744 AND FlagsEx=8192 AND RewardNextQuest=39796);
SELECT 'relations',COUNT(*) FROM creature_queststarter WHERE (id=97074 AND quest=39803) OR(id=92539 AND quest IN(39804,39796));
SELECT 'enders',COUNT(*) FROM creature_questender WHERE id=92539 AND quest IN(39803,39804);
SELECT 'buck',COUNT(*) FROM creature WHERE guid=338655 AND id=98230 AND map=1220 AND zoneId=7541 AND areaId=7803 AND ABS(position_x-3188.92)<0.01 AND ABS(position_y-1514.62)<0.01 AND ABS(position_z-181.191)<0.01;
SELECT 'havi',COUNT(*) FROM creature WHERE guid=291280 AND id=92539 AND map=1220 AND ABS(position_x-3214.38)<0.01 AND ABS(position_y-1543.61)<0.01 AND ABS(position_z-180.715)<0.01;
SELECT 'click',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=98230 AND spell_id=192171 AND cast_flags=1 AND add_npc_flag=1;
SELECT 'model',COUNT(*) FROM creature_template_wdb WHERE Entry=98230 AND Displayid1=63734 AND CursorName='questinteract';
SELECT 'flags',COUNT(*) FROM creature_template WHERE entry=98230 AND npcflag=16777216 AND unit_flags=768 AND faction=190 AND SpellStateAnimKitID=0;
SELECT 'pose',COUNT(*) FROM creature_template_addon WHERE entry=98230 AND auras='114371';
SELECT 'later-menu',COUNT(*) FROM gossip_menu_option WHERE MenuID=20539 AND OptionID=1 AND OptionBroadcastTextID=123455;
SELECT 'later-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=20539 AND SourceEntry=1 AND ConditionValue1=44721;
SELECT 'later-ai',COUNT(*) FROM smart_scripts WHERE (entryorguid=92539 AND source_type=0) OR(entryorguid=9253901 AND source_type=9);
SELECT 'conversation-lines',COUNT(*) FROM conversation_data WHERE entry=2247 AND id IN(4740,4741,4742,4743);
SELECT 'conversation-actor',COUNT(*) FROM conversation_actor WHERE entry=2247 AND actorId=53077 AND creatureId=96254 AND displayId=64208;
SELECT 'conversation-lifetime',COUNT(*) FROM conversation_actor WHERE entry=2247 AND id=0 AND duration=13550;
SELECT 'no-duplicate-link',COUNT(*) FROM spell_linked_spell WHERE spell_trigger=192171 AND spell_effect=192170;
"@
$expected=@("binding`t1","travel-empty`t0","bones-objective`t1","chain`t2","quest-definitions`t2","relations`t3","enders`t2","buck`t1","havi`t1","click`t1","model`t1","flags`t1","pose`t1","later-menu`t1","later-condition`t2","later-ai`t8","conversation-lines`t4","conversation-actor`t1","conversation-lifetime`t1","no-duplicate-link`t0")
if(!$Live){
 $sql+="DELETE FROM spell_script_names WHERE spell_id=192171; INSERT INTO spell_script_names VALUES(192171,'custom_butchery');`n$migration`nSELECT 'custom-kept',COUNT(*) FROM spell_script_names WHERE spell_id=192171 AND ScriptName='custom_butchery'; SELECT 'custom-not-merged',COUNT(*) FROM spell_script_names WHERE spell_id=192171; DELETE FROM spell_script_names WHERE spell_id=192171; UPDATE creature_template SET ScriptName='custom_buck' WHERE entry=98230;`n$migration`nSELECT 'custom-actor-not-bound',COUNT(*) FROM spell_script_names WHERE spell_id=192171; SELECT 'custom-actor-kept',COUNT(*) FROM creature_template WHERE entry=98230 AND ScriptName='custom_buck'; UPDATE creature_template SET ScriptName='',SpellStateAnimKitID=777 WHERE entry=98230; UPDATE conversation_actor SET duration=25000 WHERE entry=2247 AND id=0;`n$migration`nSELECT 'custom-duration',COUNT(*) FROM conversation_actor WHERE entry=2247 AND id=0 AND duration=25000; SELECT 'custom-kit',COUNT(*) FROM creature_template WHERE entry=98230 AND SpellStateAnimKitID=777;"
 $expected+=@("custom-kept`t1","custom-not-merged`t1","custom-actor-not-bound`t0","custom-actor-kept`t1","custom-duration`t1","custom-kit`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=$sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Bones SQL failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Bones SQL mismatch: $($actual -join '|')"}
if($Live){'PASS: installed Bones binding, native travel/objective/chain/click/pose/model, later Havi AI and conversation retained.'}else{'PASS: migration twice; native definitions, later Havi AI and custom actor/spell scripts preserved.'}
