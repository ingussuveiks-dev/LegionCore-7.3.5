-- Gathering-node interaction reaches GameObjectAI::GossipHello, represented by
-- SMART_EVENT_GOSSIP_HELLO (64). ON_SPELLCLICK (73) is creature-only.
CREATE TABLE IF NOT EXISTS `_backup_20260919_herb_gathering_events` LIKE `smart_scripts`;
CREATE TABLE IF NOT EXISTS `_backup_20260919_foxflower_ai` LIKE `gameobject_template`;

INSERT IGNORE INTO `_backup_20260919_herb_gathering_events`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` IN (241641, 244778) AND `source_type` = 1 AND `id` = 0
  AND `event_type` = 73;

INSERT IGNORE INTO `_backup_20260919_foxflower_ai`
SELECT *
FROM `gameobject_template`
WHERE `entry` = 241641 AND `type` = 50;

UPDATE `smart_scripts`
SET `event_type` = 64
WHERE `entryorguid` IN (241641, 244778) AND `source_type` = 1 AND `id` = 0
  AND `event_type` = 73;

UPDATE `gameobject_template`
SET `AIName` = 'SmartGameObjectAI'
WHERE `entry` = 241641 AND `type` = 50 AND `AIName` = '';
