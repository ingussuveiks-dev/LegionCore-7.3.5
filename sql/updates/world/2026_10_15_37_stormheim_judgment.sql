-- Judgment Day: bind the three existing bowls and rulers as one repair.
-- Preserve native templates, dialogue, scaling, SAI fallback and later241877.
-- A customized binding prevents a partial merge of this interaction.
SET @judgment_ready =
 (SELECT COUNT(*)=3 FROM gameobject_template
  WHERE entry IN(241864,241868,241869) AND type=10 AND Data0=1691 AND Data1=38811
    AND AIName IN('','SmartGameObjectAI') AND ScriptName IN('','go_stormheim_judgment'))
 AND (SELECT COUNT(*)=3 FROM creature_template
  WHERE entry IN(92889,92918,92920) AND faction=14
    AND AIName IN('','SmartAI') AND ScriptName IN('','npc_stormheim_judgment_king'))
 AND (SELECT COUNT(*)=3 FROM quest_objectives WHERE QuestID=38811 AND Type=0 AND Amount=1 AND Flags=0
    AND ((ObjectID=92889 AND StorageIndex=0) OR (ObjectID=92918 AND StorageIndex=1) OR (ObjectID=92920 AND StorageIndex=2)));

UPDATE gameobject_template SET AIName='',ScriptName='go_stormheim_judgment'
WHERE @judgment_ready AND entry IN(241864,241868,241869);
UPDATE creature_template SET AIName='',ScriptName='npc_stormheim_judgment_king'
WHERE @judgment_ready AND entry IN(92889,92918,92920);

INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 3365,'spell_stormheim_judgment_opening' WHERE @judgment_ready
 AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=3365 AND ScriptName='spell_stormheim_judgment_opening');
