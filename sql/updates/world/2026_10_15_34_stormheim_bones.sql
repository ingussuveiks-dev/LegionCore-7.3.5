-- Speaking of Bones39804: preserve the native fifteen-second butchering cast
-- and group credit; add the missing Havi conversation after its successful hit.
SET @stormheim_bones_native = EXISTS (SELECT 1 FROM creature WHERE guid=338655 AND id=98230 AND map=1220 AND zoneId=7541 AND areaId=7803)
AND EXISTS (SELECT 1 FROM npc_spellclick_spells WHERE npc_entry=98230 AND spell_id=192171 AND cast_flags=1 AND add_npc_flag=1)
AND EXISTS (SELECT 1 FROM creature WHERE guid=291280 AND id=92539 AND map=1220 AND zoneId=7541)
AND EXISTS (SELECT 1 FROM quest_objectives WHERE ID=279981 AND QuestID=39804 AND Type=0 AND ObjectID=97340 AND Amount=1 AND StorageIndex=0)
AND EXISTS (SELECT 1 FROM creature_template WHERE entry=98230 AND AIName IN('','SmartAI') AND ScriptName='')
AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=192171 AND ScriptName<>'spell_stormheim_bones_butchering');


-- A spell ID was incorrectly stored as an AnimKit ID; native114371 already
-- supplies the feign-death pose. Do not replace it with an invented animation.
UPDATE creature_template SET SpellStateAnimKitID=0
WHERE @stormheim_bones_native AND entry=98230 AND SpellStateAnimKitID=192171;

-- The native core adds5000ms to actor duration. Old3350 removed2247 at8350,
-- before its last line starts at10200. Keep the old tail budget after that line.
UPDATE conversation_actor SET duration=13550
WHERE @stormheim_bones_native AND entry=2247 AND id=0 AND actorId=53077
AND creatureId=96254 AND displayId=64208 AND duration=3350
AND EXISTS (SELECT 1 FROM conversation_data WHERE entry=2247 AND id=4743 AND textId=10200);

INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 192171,'spell_stormheim_bones_butchering' WHERE @stormheim_bones_native
AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=192171);
