-- Legion 7.3.5 level-100 boost tutorial.
-- Maps 1554/1557 are the private Alliance/Horde Boost Experience maps.
-- The moving transports are GO 246606 (TaxiPath 5528, interior 1560) and
-- GO 246607 (TaxiPath 5529, interior 1559); TransportMgr creates them per
-- instance from the client TaxiPathNode data.

START TRANSACTION;

INSERT INTO `instance_template` (`map`,`parent`,`script`,`allowMount`,`bonusChance`) VALUES
(1554,0,'instance_boost_experience_alliance',0,20),
(1557,0,'instance_boost_experience_horde',0,20)
ON DUPLICATE KEY UPDATE
`parent`=VALUES(`parent`),`script`=VALUES(`script`),
`allowMount`=VALUES(`allowMount`),`bonusChance`=VALUES(`bonusChance`);

-- The old map-only mappings are incomplete and can select the wrong class
-- scenario before the instance script knows the player's specialization.
-- The instance now constructs the exact class/spec scenario directly.
DELETE FROM `scenario_data`
WHERE `ScenarioID` IN (1073,1083,1084,1090,1091,1093,1094,1095,1096,1132,1133,1181,1182,1214)
   OR `MapID` IN (1554,1557);

UPDATE `creature_template`
SET `gossip_menu_id`=19768,`minlevel`=100,`maxlevel`=100,`npcflag`=`npcflag` | 1,
    `ScriptName`='npc_boost_trainer'
WHERE `entry` IN (100448,105317);

UPDATE `creature_template`
SET `minlevel`=100,`maxlevel`=100,`npcflag`=`npcflag` | 1,
    `ScriptName`='npc_boost_exit'
WHERE `entry` IN (107543,112565);

UPDATE `creature_template`
SET `minlevel`=100,`maxlevel`=100,`faction`=14,`ScriptName`='npc_boost_training_dummy'
WHERE `entry` IN (100648,101824,103703,104530,107104);

UPDATE `creature_template`
SET `minlevel`=100,`maxlevel`=100,`ScriptName`='npc_boost_sparring_opponent'
WHERE `entry` IN (109010,111995,111996,111998,112000);

DELETE FROM `spell_script_names` WHERE `spell_id`=227058 OR `ScriptName`='spell_q42740';
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES (227058,'spell_q42740');

COMMIT;
