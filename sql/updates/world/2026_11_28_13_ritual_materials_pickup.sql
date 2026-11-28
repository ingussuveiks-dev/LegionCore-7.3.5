-- Let the native create-item spells confirm a successful personal gathering.
UPDATE `creature_template`
SET `ScriptName` = 'npc_darkshore_ritual_material_source'
WHERE `entry` IN (33127, 33313, 33978) AND `AIName` = 'SmartAI' AND `ScriptName` = '';

UPDATE `conditions`
SET `ScriptName` = 'condition_darkshore_ritual_material'
WHERE `SourceTypeOrReferenceId` = 18 AND `SourceId` = 0 AND `ElseGroup` = 0
  AND (`SourceGroup`, `SourceEntry`) IN ((33127,62512), (33313,62788), (33978,64116))
  AND `ConditionTypeOrReference` = 9 AND `ConditionTarget` = 0
  AND `ConditionValue1` = 13566 AND `NegativeCondition` = 0 AND `ScriptName` = '';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
SELECT spell.`id`, 'spell_darkshore_gather_ritual_material'
FROM (SELECT 62512 AS `id` UNION ALL SELECT 62788 UNION ALL SELECT 64116) AS spell
WHERE NOT EXISTS (SELECT 1 FROM `spell_script_names` AS existing
    WHERE existing.`spell_id` = spell.`id` AND existing.`ScriptName` = 'spell_darkshore_gather_ritual_material');

DELETE FROM `smart_scripts`
WHERE `entryorguid` IN (33127, 33313, 33978) AND `source_type` = 0 AND `id` = 0 AND `link` = 1
  AND `event_type` = 73 AND `event_chance` = 100 AND `event_flags` = 0
  AND `action_type` = 41 AND `action_param1` = 0 AND `target_type` = 1
  AND `comment` = 'SpellClick - Despawn';
