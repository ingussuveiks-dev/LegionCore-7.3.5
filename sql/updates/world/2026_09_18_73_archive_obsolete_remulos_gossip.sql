-- These linked actions listen for gossip menu 10215 option 2, but the 7.3.5
-- database defines only options 0 and 1. Their item 90001 is also absent from
-- the 7.3.5.26972 client ItemSparse data and from every quest using Remulos.
-- The pair is therefore unreachable residue from removed custom/holiday data.
CREATE TABLE IF NOT EXISTS `_backup_20260918_obsolete_remulos_gossip` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_obsolete_remulos_gossip`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 11832
  AND `source_type` = 0
  AND `id` IN (4, 5)
  AND ((`id` = 4 AND `event_type` = 62 AND `event_param1` = 10215 AND `event_param2` = 2)
    OR (`id` = 5 AND `event_type` = 61 AND `action_type` = 56 AND `action_param1` = 90001));

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 11832
  AND `source_type` = 0
  AND `id` IN (4, 5)
  AND ((`id` = 4 AND `event_type` = 62 AND `event_param1` = 10215 AND `event_param2` = 2)
    OR (`id` = 5 AND `event_type` = 61 AND `action_type` = 56 AND `action_param1` = 90001));
