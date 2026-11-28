-- Slowing the Inevitable 24920. Apply with the matching bat flight scripts.
START TRANSACTION;

SET @bat_correct_source = (
    EXISTS (SELECT 1 FROM `creature` WHERE `guid`=149342 AND `id`=38615 AND `map`=654)
    AND EXISTS (SELECT 1 FROM `creature_template` WHERE `entry`=38540 AND `VehicleId`=641
        AND `spell1`=72246 AND `ScriptName`='npc_captured_riding_bat')
    AND EXISTS (SELECT 1 FROM `conditions` WHERE `SourceTypeOrReferenceId`=18 AND `SourceGroup`=38615
        AND `SourceEntry`=72472 AND `ElseGroup`=0 AND `ConditionTypeOrReference`=9 AND `ConditionValue1`=24920)
    AND (SELECT COUNT(*) FROM `script_waypoint` WHERE `entry`=38540)=36
    AND EXISTS (SELECT 1 FROM `quest_objectives` WHERE `ID`=265634 AND `QuestID`=24920 AND `ObjectID`=38287 AND `Amount`=6)
    AND EXISTS (SELECT 1 FROM `quest_objectives` WHERE `ID`=265635 AND `QuestID`=24920 AND `ObjectID`=38363 AND `Amount`=40)
);
SET @bat_source = @bat_correct_source AND
    EXISTS (SELECT 1 FROM `creature_template` WHERE `entry`=38540 AND `spell2`=72849 AND `spell5`=0) AND
    EXISTS (SELECT 1 FROM `npc_spellclick_spells` WHERE `npc_entry`=38615 AND `spell_id`=72472
        AND `cast_flags`=3 AND `user_type`=0 AND `add_npc_flag`=1) AND
    NOT EXISTS (SELECT 1 FROM `spell_script_names` WHERE `spell_id` IN (72246,72247,72849));
SET @bat_installed = @bat_correct_source AND
    EXISTS (SELECT 1 FROM `creature_template` WHERE `entry`=38540 AND `spell2`=0 AND `spell5`=72849) AND
    EXISTS (SELECT 1 FROM `npc_spellclick_spells` WHERE `npc_entry`=38615 AND `spell_id`=72472 AND `cast_flags`=1) AND
    (SELECT COUNT(*) FROM `spell_script_names` WHERE (`spell_id`,`ScriptName`) IN
        ((72246,'spell_gilneas_bat_iron_bomb'),(72247,'spell_gilneas_bat_bomb_impact'),(72849,'spell_gilneas_bat_fly_back'))) = 3;
SET @bat_error = IF(@bat_source OR @bat_installed, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Gilneas bat 24920 source differs from checked data''');
PREPARE gilneas_bat_check FROM @bat_error;
EXECUTE gilneas_bat_check;
DEALLOCATE PREPARE gilneas_bat_check;

-- Keep the clicker as caster. Preserve the actual clicked bat as the target
-- before the destination-only summon normalizes its explicit target flags.
UPDATE `npc_spellclick_spells` SET `cast_flags`=1
WHERE `npc_entry`=38615 AND `spell_id`=72472 AND @bat_source;

-- The Legion vehicle bar uses buttons 1 and 5.
UPDATE `creature_template` SET `spell2`=0, `spell5`=72849 WHERE `entry`=38540 AND @bat_source;

INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`)
SELECT 72246,'spell_gilneas_bat_iron_bomb' WHERE @bat_source
UNION ALL SELECT 72247,'spell_gilneas_bat_bomb_impact' WHERE @bat_source
UNION ALL SELECT 72849,'spell_gilneas_bat_fly_back' WHERE @bat_source;

COMMIT;
