-- The eight capital mascots share the original kit gossip and native creation spell.
-- Keep the configured yearly cycle available through the other world events' calendar horizon.
UPDATE `game_event` SET `end_time`='2035-03-01 00:00:00'
WHERE `eventEntry`=8 AND `holiday`=423 AND `world_event`=0 AND `end_time`='2021-02-20 20:00:00';

UPDATE `creature_template` SET `npcflag`=(`npcflag` | 1), `AIName`='SmartAI'
WHERE `entry` IN (37887,38039,38040,38041,38042,38043,38044,38045)
  AND `gossip_menu_id`=10948 AND `ScriptName`='' AND `AIName` IN ('','SmartAI');

INSERT IGNORE INTO `smart_scripts`
(`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_param1`,`event_param2`,
 `action_type`,`action_param1`,`target_type`,`comment`)
SELECT `entry`,0,0,1,'',62,10948,0,11,70648,7,'Peddlefeet - Kit gossip option - Create the kit for the selecting player'
FROM `creature_template` WHERE `entry` IN (37887,38039,38040,38041,38042,38043,38044,38045)
  AND `gossip_menu_id`=10948 AND `ScriptName`='' AND `AIName`='SmartAI';

INSERT IGNORE INTO `smart_scripts`
(`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`action_type`,`target_type`,`comment`)
SELECT `entry`,0,1,0,'',61,72,7,'Peddlefeet - Linked kit selection - Close gossip'
FROM `creature_template` WHERE `entry` IN (37887,38039,38040,38041,38042,38043,38044,38045)
  AND `gossip_menu_id`=10948 AND `ScriptName`='' AND `AIName`='SmartAI';

INSERT IGNORE INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,
 `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`Comment`)
VALUES
(15,10948,0,0,0,12,0,8,0,0,0,'Offer the kit while Love is in the Air is active'),
(15,10948,0,0,0,2,0,49661,1,1,1,'Offer the unique kit only when the player does not own one'),
(17,0,70648,0,0,12,0,8,0,0,0,'Create the kit only during Love is in the Air'),
(17,0,69511,0,0,2,0,49661,1,0,0,'Charm proc requires its caster to carry the collectors kit'),
(13,1,69511,0,0,2,0,49661,1,0,0,'Each native charm recipient must carry their own collectors kit');

INSERT IGNORE INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,
 `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`Comment`)
SELECT `sources`.`source`,0,`quest`.`ID`,0,0,12,0,8,0,0,0,'Lovely Charm Bracelet gift requires Love is in the Air'
FROM (SELECT 19 AS `source` UNION ALL SELECT 20) AS `sources`
CROSS JOIN `quest_template` AS `quest`
WHERE `quest`.`ID` IN (24597,24609,24610,24611,24612,24613,24614,24615) AND `quest`.`StartItem`=49661;

UPDATE `quest_template` SET `RewardItem2`=49927, `RewardAmount2`=5
WHERE `ID` IN (24597,24609,24610,24611,24612,24613,24614,24615) AND `StartItem`=49661
  AND `RewardItem2`=0 AND `RewardAmount2`=0;

-- Legion's city recipients are Anduin and Saurfang. Use the existing throne-room phases.
DELETE FROM `creature_questender` WHERE (`quest`=24597 AND `id`=29611)
  OR (`quest`=24612 AND `id` IN (39605,86832));

INSERT IGNORE INTO `creature_questender` (`id`,`quest`)
SELECT `id`,24597 FROM `creature`
WHERE `zoneId`=1519 AND `areaId`=6292 AND `id` IN (100429,109466)
GROUP BY `id`;

UPDATE `quest_template`
SET `LogDescription`=REPLACE(`LogDescription`,'King Varian Wrynn','King Anduin Wrynn'),
    `QuestCompletionLog`=REPLACE(`QuestCompletionLog`,'King Varian Wrynn','King Anduin Wrynn')
WHERE `ID`=24597 AND `StartItem`=49661;

UPDATE `quest_template`
SET `LogTitle`='A Gift for High Overlord Saurfang',
    `LogDescription`=REPLACE(`LogDescription`,'the Warchief','High Overlord Saurfang'),
    `QuestDescription`=REPLACE(`QuestDescription`,'the Warchief','High Overlord Saurfang'),
    `QuestCompletionLog`=REPLACE(`QuestCompletionLog`,'the Warchief','High Overlord Saurfang')
WHERE `ID`=24612 AND `StartItem`=49661;
