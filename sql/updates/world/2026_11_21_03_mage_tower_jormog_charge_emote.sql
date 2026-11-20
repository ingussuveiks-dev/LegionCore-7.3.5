START TRANSACTION;

-- The original charge warning uses CHAT_MSG_MONSTER_EMOTE.
UPDATE `creature_text`
SET `Type` = 16
WHERE `CreatureID` = 117484 AND `GroupID` = 0 AND `ID` = 0
  AND `Type` = 41 AND `BroadcastTextID` = 130585;

COMMIT;
