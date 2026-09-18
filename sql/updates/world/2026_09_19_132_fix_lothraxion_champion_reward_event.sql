-- The comment "QR - SC" describes Quest Reward -> Summon Conversation.
-- Lothraxion starts and ends quest 43701 (Champion: Lothraxion), so this row's
-- imported distance/eventobject trigger with empty parameters is corruption.
CREATE TABLE IF NOT EXISTS `_backup_20260919_lothraxion_champion_reward_event` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260919_lothraxion_champion_reward_event`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 109105 AND `source_type` = 0 AND `id` = 0
  AND `event_type` = 89 AND `action_type` = 206 AND `action_param1` = 3641;

UPDATE `smart_scripts`
SET `link` = 0,
    `event_type` = 20,
    `event_param1` = 43701,
    `comment` = 'Lothraxion - On Quest 43701 Rewarded - Summon Conversation 3641'
WHERE `entryorguid` = 109105 AND `source_type` = 0 AND `id` = 0
  AND `event_type` = 89 AND `action_type` = 206 AND `action_param1` = 3641;
