-- These targets are independently initialized UPDATE_IC combat timers, not
-- LINK continuations. Keep both timer rows and all story/summon/DATA actions.
-- Never turn the combat rows into LINK events or cast them at intro time.
CREATE TEMPORARY TABLE `niskara_combat_link_20261117` (
 `entry` INT UNSIGNED PRIMARY KEY, `root_id` INT UNSIGNED,
 `root_link` INT UNSIGNED, `root_event` INT UNSIGNED, `root_action` INT UNSIGNED,
 `root_p1` INT UNSIGNED, `root_p2` INT UNSIGNED, `root_target` INT UNSIGNED,
 `combat_spell` INT UNSIGNED
);
INSERT INTO `niskara_combat_link_20261117` VALUES
 (109357,0,1,38,53,1,109357,1,197639),
 (109108,6,7,61,45,3,3,19,202817),
 (109109,6,7,61,45,3,3,19,202817);
UPDATE `smart_scripts` root
JOIN `niskara_combat_link_20261117` expected ON expected.`entry`=root.`entryorguid`
JOIN `creature_template` actor ON actor.`entry`=expected.`entry` AND actor.`AIName`='SmartAI' AND actor.`ScriptName`=''
JOIN `smart_scripts` combat ON combat.`entryorguid`=root.`entryorguid` AND combat.`source_type`=0 AND combat.`id`=expected.`root_link`
SET root.`link`=0
WHERE root.`source_type`=0 AND root.`id`=expected.`root_id` AND root.`link`=expected.`root_link`
 AND root.`event_type`=expected.`root_event` AND root.`event_chance`=100 AND root.`event_flags`=1
 AND root.`action_type`=expected.`root_action` AND root.`action_param1`=expected.`root_p1` AND root.`action_param2`=expected.`root_p2` AND root.`target_type`=expected.`root_target`
 AND combat.`event_type`=0 AND combat.`event_phase_mask`=0 AND combat.`event_chance`=100 AND combat.`event_flags`=0
 AND combat.`event_param1`=3000 AND combat.`event_param2`=5000 AND combat.`event_param3`=4000 AND combat.`event_param4`=6000
 AND combat.`link`=0 AND combat.`action_type`=11 AND combat.`action_param1`=expected.`combat_spell` AND combat.`target_type`=2;
DROP TEMPORARY TABLE `niskara_combat_link_20261117`;
