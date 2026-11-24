-- Summon Da-Nel (193978) has unlimited native duration. Keep the personal follow
-- chain and the five-second acceptance cleanup, while the AI owns the offer lifetime.
START TRANSACTION;

UPDATE `creature_template` AS `giver`
JOIN `smart_scripts` AS `follow` ON `follow`.`entryorguid`=`giver`.`entry` AND `follow`.`source_type`=0 AND `follow`.`id`=0
JOIN `smart_scripts` AS `timer` ON `timer`.`entryorguid`=`giver`.`entry` AND `timer`.`source_type`=0 AND `timer`.`id`=3
JOIN `smart_scripts` AS `personal` ON `personal`.`entryorguid`=`giver`.`entry` AND `personal`.`source_type`=0 AND `personal`.`id`=5
JOIN `smart_scripts` AS `acceptance` ON `acceptance`.`entryorguid`=`giver`.`entry` AND `acceptance`.`source_type`=0 AND `acceptance`.`id`=1
SET `giver`.`ScriptName`='npc_dalaran_da_nel', `follow`.`link`=5
WHERE `giver`.`entry`=98519 AND `giver`.`AIName`='SmartAI' AND `giver`.`ScriptName`=''
  AND `follow`.`event_type`=54 AND `follow`.`action_type`=29 AND `follow`.`target_type`=7 AND `follow`.`link`=3
  AND `timer`.`event_type`=61 AND `timer`.`action_type`=41 AND `timer`.`action_param1`=30000 AND `timer`.`target_type`=1 AND `timer`.`link`=5
  AND `personal`.`event_type`=61 AND `personal`.`action_type`=207 AND `personal`.`target_type`=7 AND `personal`.`link`=0
  AND `acceptance`.`event_type`=19 AND `acceptance`.`event_param1`=12103 AND `acceptance`.`action_type`=41
  AND `acceptance`.`action_param1`=5000 AND `acceptance`.`link`=2;

DELETE `timer` FROM `smart_scripts` AS `timer`
JOIN `creature_template` AS `giver` ON `giver`.`entry`=`timer`.`entryorguid` AND `giver`.`ScriptName`='npc_dalaran_da_nel'
JOIN `smart_scripts` AS `follow` ON `follow`.`entryorguid`=`giver`.`entry` AND `follow`.`source_type`=0 AND `follow`.`id`=0 AND `follow`.`link`=5
LEFT JOIN `smart_scripts` AS `inbound` ON `inbound`.`entryorguid`=`timer`.`entryorguid` AND `inbound`.`source_type`=0 AND `inbound`.`link`=3
WHERE `timer`.`entryorguid`=98519 AND `timer`.`source_type`=0 AND `timer`.`id`=3 AND `inbound`.`id` IS NULL
  AND `timer`.`event_type`=61 AND `timer`.`action_type`=41 AND `timer`.`action_param1`=30000 AND `timer`.`target_type`=1 AND `timer`.`link`=5;

COMMIT;
