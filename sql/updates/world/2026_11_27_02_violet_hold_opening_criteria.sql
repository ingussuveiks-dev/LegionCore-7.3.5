-- Map 1494 records event 48801 using a participating mage after Tomb of Ice lands.
-- Keep the original freeze and retire only its obsolete victim-targeted credit.
START TRANSACTION;

UPDATE `smart_scripts` AS freeze
JOIN `smart_scripts` AS leaf ON leaf.`entryorguid` = freeze.`entryorguid` AND leaf.`source_type` = freeze.`source_type`
    AND leaf.`id` = freeze.`link` AND leaf.`id` = 3 AND leaf.`link` = 0
SET freeze.`link` = 0
WHERE freeze.`entryorguid` = 102785 AND freeze.`source_type` = 0 AND freeze.`id` = 2
  AND freeze.`event_type` = 2 AND freeze.`action_type` = 75 AND freeze.`action_param1` = 211589 AND freeze.`target_type` = 1
  AND leaf.`event_type` = 61 AND leaf.`action_type` = 205 AND leaf.`action_param1` = 92
  AND leaf.`action_param2` = 48801 AND leaf.`target_type` = 2;

DELETE leaf
FROM `smart_scripts` AS leaf
JOIN `smart_scripts` AS freeze ON freeze.`entryorguid` = leaf.`entryorguid` AND freeze.`source_type` = leaf.`source_type`
WHERE leaf.`entryorguid` = 102785 AND leaf.`source_type` = 0 AND leaf.`id` = 3 AND leaf.`link` = 0
  AND leaf.`event_type` = 61 AND leaf.`action_type` = 205 AND leaf.`action_param1` = 92 AND leaf.`action_param2` = 48801
  AND leaf.`target_type` = 2 AND freeze.`id` = 2 AND freeze.`link` = 0 AND freeze.`action_type` = 75 AND freeze.`action_param1` = 211589;

COMMIT;
