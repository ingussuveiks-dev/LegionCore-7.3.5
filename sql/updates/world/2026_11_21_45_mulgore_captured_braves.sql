UPDATE `gameobject_template` SET `ScriptName` = 'go_mulgore_quilboar_cage'
WHERE `entry` = 202112 AND `type` = 10 AND `Data1` = 24852 AND `AIName` = '' AND `ScriptName` = '';

UPDATE `creature_template` SET `ScriptName` = 'npc_mulgore_captured_brave'
WHERE `entry` = 38345 AND `AIName` = '' AND `ScriptName` = '';

-- Keep the captives inside their original cages until the door is opened.
UPDATE `creature` SET `spawndist` = 0, `MovementType` = 0
WHERE `id` = 38345 AND `map` = 1 AND `areaId` = 4878 AND `MovementType` = 1
  AND `guid` IN (93501, 93664, 93706, 169785, 169830, 169831);

INSERT INTO `creature_text`
    (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `BroadcastTextId`, `comment`)
VALUES
    (38345, 0, 0, '', 12, 0, 50, 38278, 'Captured Brave - released from the cage'),
    (38345, 0, 1, '', 12, 0, 50, 38279, 'Captured Brave - released from the cage');
