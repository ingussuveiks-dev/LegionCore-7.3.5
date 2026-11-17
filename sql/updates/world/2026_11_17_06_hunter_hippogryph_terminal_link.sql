-- The matching26972 base dump and live102177 script contain exactly the
-- credit and teleport nodes. Teleport is terminal; its LINK2 has no node.
-- Preserve both real actions and their admission, rather than fabricating
-- a quest reward/arrival action or suppressing SmartScript diagnostics.
SET @hunter_bird_terminal_safe := (
    (SELECT COUNT(*) FROM `creature_template` WHERE `entry`=102177
        AND `AIName`='SmartAI' AND `ScriptName`='') = 1
    AND (SELECT COUNT(*) FROM `smart_scripts` WHERE `entryorguid`=102177 AND `source_type`=0) = 2
    AND EXISTS (SELECT 1 FROM `smart_scripts` WHERE `entryorguid`=102177 AND `source_type`=0
        AND `id`=0 AND `link`=1 AND `event_type`=73 AND `event_phase_mask`=0
        AND `event_chance`=100 AND `event_flags`=0 AND `Difficulties`=''
        AND `action_type`=33 AND `action_param1`=102173 AND `action_param2`=0
        AND `action_param3`=0 AND `action_param4`=0 AND `action_param5`=0 AND `action_param6`=0
        AND `target_type`=7)
    AND EXISTS (SELECT 1 FROM `smart_scripts` WHERE `entryorguid`=102177 AND `source_type`=0
        AND `id`=1 AND `link` IN(0,2) AND `event_type`=61 AND `event_phase_mask`=0
        AND `event_chance`=100 AND `event_flags`=0 AND `Difficulties`=''
        AND `action_type`=62 AND `action_param1`=1220 AND `action_param2`=0
        AND `action_param3`=0 AND `action_param4`=0 AND `action_param5`=0 AND `action_param6`=0
        AND `target_type`=7 AND `target_param1`=0 AND `target_param2`=0
        AND `target_param3`=0 AND `target_param4`=0
        AND ABS(`target_x`+858.293)<0.001 AND ABS(`target_y`-3589.13)<0.001
        AND ABS(`target_z`-1.55252)<0.001 AND ABS(`target_o`-1.59359)<0.001)
    AND EXISTS (SELECT 1 FROM `conditions` WHERE `SourceTypeOrReferenceId`=22
        AND `SourceGroup`=1 AND `SourceEntry`=102177 AND `SourceId`=0 AND `ElseGroup`=0
        AND `ConditionTypeOrReference`=9 AND `ConditionTarget`=0
        AND `ConditionValue1`=40392 AND `NegativeCondition`=0)
);
SET @hunter_bird_terminal_guard := IF(@hunter_bird_terminal_safe, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Hunter hippogryph terminal graph differs''');
PREPARE hunter_bird_terminal_check FROM @hunter_bird_terminal_guard;
EXECUTE hunter_bird_terminal_check;
DEALLOCATE PREPARE hunter_bird_terminal_check;

UPDATE `smart_scripts` SET `link`=0
WHERE @hunter_bird_terminal_safe
    AND `entryorguid`=102177 AND `source_type`=0 AND `id`=1 AND `link`=2;

SET @hunter_bird_terminal_safe := NULL;
SET @hunter_bird_terminal_guard := NULL;
