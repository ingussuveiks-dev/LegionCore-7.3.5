-- Retain the native combat/ambient AI outside the scoped scene factory.
SET @VREKT_SCENE_BINDINGS = (
  SELECT COUNT(*)=3 FROM `creature_template`
  WHERE (`entry`=115846 AND `AIName` IN('','AggressorAI') AND `ScriptName` IN('npc_vrekt','npc_vrekt_scene'))
     OR (`entry`=107830 AND `AIName`='SmartAI' AND `ScriptName` IN('','npc_vrekt_scene'))
     OR (`entry`=115885 AND `AIName`='' AND `ScriptName` IN('','npc_vrekt_scene'))
);
UPDATE `creature_template` SET `ScriptName`='npc_vrekt_scene'
WHERE @VREKT_SCENE_BINDINGS AND `entry`=115846 AND `AIName` IN('','AggressorAI') AND `ScriptName` IN('npc_vrekt','npc_vrekt_scene');
UPDATE `creature_template` SET `ScriptName`='npc_vrekt_scene'
WHERE @VREKT_SCENE_BINDINGS AND `entry`=107830 AND `AIName`='SmartAI' AND `ScriptName` IN('','npc_vrekt_scene');
UPDATE `creature_template` SET `ScriptName`='npc_vrekt_scene'
WHERE @VREKT_SCENE_BINDINGS AND `entry`=115885 AND `AIName`='' AND `ScriptName` IN('','npc_vrekt_scene');

-- This exact imported scene row belongs to the captured WQ replacement.
-- Its former ordinary village caster now keeps its native non-WQ lifecycle.
DELETE FROM `smart_scripts`
WHERE @VREKT_SCENE_BINDINGS AND `entryorguid`=107830 AND `source_type`=0 AND `id`=0 AND `link`=0
  AND `Difficulties`='' AND `event_type`=1 AND `event_phase_mask`=0
  AND `event_chance`=100 AND `event_flags`=1
  AND `event_param1`=0 AND `event_param2`=0 AND `event_param3`=0 AND `event_param4`=0 AND `event_param5`=0
  AND `action_type`=11 AND `action_param1`=211409 AND `action_param2`=0
  AND `action_param3`=0 AND `action_param4`=0 AND `action_param5`=0 AND `action_param6`=0
  AND `target_type`=19 AND `target_param1`=115846 AND `target_param2`=0 AND `target_param3`=0 AND `target_param4`=0
  AND `target_x`=0 AND `target_y`=0 AND `target_z`=0 AND `target_o`=0 AND `comment`='OOC - CS';
