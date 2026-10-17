-- The existing193637 dummy has no release handler. Scope completed Opening
-- to the four original circles; preserve native data, actors and other users.
SET @wings_release_repair :=
 (SELECT COUNT(*) FROM gameobject_template WHERE entry IN(240650,244335,244336,244337)
  AND type=10 AND displayId=16973 AND ABS(size-0.4)<.001 AND Data0=1690 AND Data1=0 AND Data10=193637
  AND ScriptName IN('','go_stormheim_wings'))=4
 AND (SELECT COUNT(*) FROM creature_template WHERE entry IN(97028,97031,97032,97033) AND ScriptName='' AND AIName='')=4
 AND (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=38413)=4
 AND (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=38413 AND Type=2 AND Amount=1 AND Flags=0 AND
  ((ObjectID=240650 AND StorageIndex=1) OR(ObjectID=244335 AND StorageIndex=2) OR(ObjectID=244336 AND StorageIndex=3) OR(ObjectID=244337 AND StorageIndex=4)))=4
 AND (SELECT COUNT(*) FROM gameobject WHERE map=1220 AND PhaseId='' AND
  ((guid=115480 AND id=240650 AND ABS(position_x-2033.69)<.01 AND ABS(position_y-2186.11)<.01 AND ABS(position_z-465.141)<.01) OR
   (guid=115466 AND id=244335 AND ABS(position_x-2133.51)<.01 AND ABS(position_y-2373.01)<.01 AND ABS(position_z-490.76)<.01) OR
   (guid=115473 AND id=244336 AND ABS(position_x-2058.21)<.01 AND ABS(position_y-2475.53)<.01 AND ABS(position_z-484.165)<.01) OR
   (guid=115476 AND id=244337 AND ABS(position_x-1998)<.01 AND ABS(position_y-2383.88)<.01 AND ABS(position_z-482.131)<.01)))=4
 AND (SELECT COUNT(*) FROM creature WHERE map=1220 AND PhaseId='' AND
  ((guid=146706136 AND id=97028 AND ABS(position_x-2033.7)<.01 AND ABS(position_y-2186.07)<.01 AND ABS(position_z-465.226)<.01) OR
   (guid=146706113 AND id=97031 AND ABS(position_x-2132.36)<.01 AND ABS(position_y-2372.49)<.01 AND ABS(position_z-490.706)<.01) OR
   (guid=146706114 AND id=97032 AND ABS(position_x-2057.1)<.01 AND ABS(position_y-2473.28)<.01 AND ABS(position_z-483.957)<.01) OR
   (guid=146706125 AND id=97033 AND ABS(position_x-1998.81)<.01 AND ABS(position_y-2385.1)<.01 AND ABS(position_z-482.035)<.01)))=4;
UPDATE gameobject_template SET ScriptName='go_stormheim_wings'
WHERE entry IN(240650,244335,244336,244337) AND @wings_release_repair;
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 3365,'spell_stormheim_wings_opening' WHERE @wings_release_repair
AND NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=3365 AND ScriptName='spell_stormheim_wings_opening');
