-- Drop the obsolete spawn display so these Kodos use their Legion template.
UPDATE `creature` AS spawn
JOIN `creature_template_wdb` AS model ON model.`Entry` = spawn.`id`
JOIN `creature_model_info` AS info ON info.`DisplayID` = model.`Displayid1`
SET spawn.`modelid` = 0
WHERE spawn.`map` = 571 AND spawn.`modelid` = 23646 AND info.`DisplayID` = 30562
  AND ((spawn.`id` = 26160 AND spawn.`guid` IN (45811,45812,45814,45815,45819))
    OR (spawn.`id` = 26488 AND spawn.`guid` IN (51891,51892,51965,51966)));
