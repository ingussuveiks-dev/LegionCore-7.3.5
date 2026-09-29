-- Bulzan's only SmartAI row already had this dangling link in the original
-- 7.3.5 world dump. Keep his existing self-cast, and remove the link only while
-- its destination is absent; do not overwrite a subsequently restored chain.
UPDATE `smart_scripts` AS `source`
LEFT JOIN `smart_scripts` AS `linked`
    ON `linked`.`entryorguid` = `source`.`entryorguid`
    AND `linked`.`source_type` = `source`.`source_type`
    AND `linked`.`id` = `source`.`link`
SET `source`.`link` = 0
WHERE `source`.`entryorguid` = 99702 AND `source`.`source_type` = 0
    AND `source`.`id` = 0 AND `source`.`link` = 1
    AND `source`.`event_type` = 60 AND `source`.`action_type` = 11
    AND `source`.`action_param1` = 196229 AND `source`.`target_type` = 1
    AND `linked`.`id` IS NULL;
