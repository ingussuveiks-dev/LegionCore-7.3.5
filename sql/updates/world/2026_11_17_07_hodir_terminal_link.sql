-- Hodir's one-time3% health-floor setup is a complete action. Both the
-- matching26972 base and live graph have no event6; its combat/Thorim
-- surrender progression uses the separate7->8->12 and9->10->13 paths.
SET @hodir_terminal_safe := (
    (SELECT COUNT(*) FROM `creature_template` WHERE `entry`=109704
        AND `AIName`='SmartAI' AND `ScriptName`='')=1
    AND (SELECT COUNT(*) FROM `smart_scripts` WHERE `entryorguid`=109704 AND `source_type`=0)=13
    AND NOT EXISTS (SELECT 1 FROM `smart_scripts` WHERE `entryorguid`=109704 AND `source_type`=0 AND `id`=6)
    AND EXISTS (SELECT 1 FROM `smart_scripts` WHERE `entryorguid`=109704 AND `source_type`=0 AND `id`=5
        AND `link` IN(0,6) AND `Difficulties`='' AND `event_type`=60 AND `event_phase_mask`=0
        AND `event_chance`=100 AND `event_flags`=1 AND `event_param1`=0 AND `event_param2`=0
        AND `event_param3`=0 AND `event_param4`=0 AND `event_param5`=0
        AND `action_type`=42 AND `action_param1`=0 AND `action_param2`=3
        AND `action_param3`=0 AND `action_param4`=0 AND `action_param5`=0 AND `action_param6`=0
        AND `target_type`=1 AND `target_param1`=0 AND `target_param2`=0
        AND `target_param3`=0 AND `target_param4`=0
        AND `target_x`=0 AND `target_y`=0 AND `target_z`=0 AND `target_o`=0)
    AND EXISTS (SELECT 1 FROM `conditions` WHERE `SourceTypeOrReferenceId`=22
        AND `SourceGroup`=1 AND `SourceEntry`=109704 AND `SourceId`=0 AND `ElseGroup`=0
        AND `ConditionTypeOrReference`=44 AND `ConditionTarget`=1
        AND `ConditionValue1`=1146 AND `ConditionValue2`=5 AND `NegativeCondition`=0)
);
SET @hodir_terminal_guard := IF(@hodir_terminal_safe, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Hodir terminal graph differs''');
PREPARE hodir_terminal_check FROM @hodir_terminal_guard;
EXECUTE hodir_terminal_check;
DEALLOCATE PREPARE hodir_terminal_check;

UPDATE `smart_scripts` SET `link`=0
WHERE @hodir_terminal_safe
    AND `entryorguid`=109704 AND `source_type`=0 AND `id`=5 AND `link`=6;

SET @hodir_terminal_safe := NULL;
SET @hodir_terminal_guard := NULL;
