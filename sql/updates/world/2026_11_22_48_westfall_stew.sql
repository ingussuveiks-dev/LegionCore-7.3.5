-- The reusable Westfall Stew item has a native summon effect. Gate the real summon at cast time.
INSERT IGNORE INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
    (79450,'spell_offer_westfall_stew');

-- Full Belly is earned when fed. Preserve the resident's other original ambient aura.
UPDATE `creature_template_addon` SET `auras`='78677'
WHERE `entry`=42384 AND `auras`='79451 78677';

-- Reuse the original Westfall feeding BroadcastTexts. Other resident dialogue groups serve separate quests.
INSERT INTO `creature_text`
    (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`BroadcastTextID`,
     `MinTimer`,`MaxTimer`,`SpellID`,`comment`)
SELECT `resident`.`entry`,5,`line`.`ID`,'',12,0,100,0,0,0,`line`.`BroadcastTextID`,0,0,0,'Westfall Stew - resident fed'
FROM `creature_template` AS `resident`
CROSS JOIN (
    SELECT 0 AS `ID`,42486 AS `BroadcastTextID`
    UNION ALL SELECT 1,42487
    UNION ALL SELECT 2,42488
    UNION ALL SELECT 3,42489
    UNION ALL SELECT 4,42490
    UNION ALL SELECT 5,42491
) AS `line`
WHERE `resident`.`entry` IN (42383,42384,42386,42391,42400)
ON DUPLICATE KEY UPDATE
    `Type`=VALUES(`Type`), `Language`=VALUES(`Language`), `Probability`=VALUES(`Probability`),
    `Emote`=VALUES(`Emote`), `Duration`=VALUES(`Duration`), `Sound`=VALUES(`Sound`),
    `BroadcastTextID`=VALUES(`BroadcastTextID`), `MinTimer`=VALUES(`MinTimer`), `MaxTimer`=VALUES(`MaxTimer`),
    `SpellID`=VALUES(`SpellID`), `comment`=VALUES(`comment`);
