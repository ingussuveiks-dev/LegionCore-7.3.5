-- 39385: refresh the existing native detonator creator after GO credit.
SET @gift_repair=(
 (SELECT COUNT(*) FROM gameobject_template WHERE entry=243455 AND type=10 AND Data0=1691 AND Data1=0 AND Data2=0 AND Data10=0 AND Data14=51667 AND ScriptName IN('','go_stormheim_gift_planting'))=1
 AND (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=39385 AND ((Type=2 AND ObjectID=243455 AND Amount=3 AND StorageIndex=0) OR (Type=0 AND ObjectID=95412 AND Amount=1 AND StorageIndex=1)))=2
 AND (SELECT COUNT(*) FROM spell_area WHERE spell=189152 AND area=7773 AND quest_start=39385 AND autocast=1 AND quest_start_status=8)=1
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=243455 AND source_type=1)
 AND NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=189134 AND ScriptName<>'spell_stormheim_gift_detonator')
 AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=189134)
 AND NOT EXISTS(SELECT 1 FROM spell_target_position WHERE ID=189134)
);
UPDATE gameobject_template SET ScriptName='go_stormheim_gift_planting' WHERE @gift_repair AND entry=243455;
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 189134,'spell_stormheim_gift_detonator' WHERE @gift_repair AND NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=189134 AND ScriptName='spell_stormheim_gift_detonator');
