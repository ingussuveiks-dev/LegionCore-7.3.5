-- The original UWoW store scripts are no longer part of this core. Their
-- multi-vendor, item-return and all-in-one donation NPCs therefore cannot do
-- anything and have no valid creature display records. BattlePay replaces
-- those services, so remove the orphaned spawns and templates.
DELETE FROM `creature` WHERE `id` IN (220010, 220011, 230000);
DELETE FROM `creature_text` WHERE `CreatureID` IN (220010, 220011, 230000);
DELETE FROM `creature_template_wdb_locale` WHERE `ID` IN (220010, 220011, 230000);
DELETE FROM `creature_template_wdb` WHERE `Entry` IN (220010, 220011, 230000);
DELETE FROM `creature_template` WHERE `Entry` IN (220010, 220011, 230000);

-- Jarvis is the one useful survivor: the core still supports persistent
-- per-account XP rates. Restore a valid client-visible definition and bind the
-- small replacement gossip script instead of the removed donation bundle.
UPDATE `creature_template`
SET `minlevel` = 110,
    `maxlevel` = 110,
    `ScriptName` = 'npc_change_rates'
WHERE `Entry` = 230007;

DELETE FROM `creature_template_wdb_locale` WHERE `ID` = 230007;
DELETE FROM `creature_template_wdb` WHERE `Entry` = 230007;
INSERT INTO `creature_template_wdb`
    (`Entry`, `Name1`, `Name2`, `Name3`, `Name4`,
     `NameAlt1`, `NameAlt2`, `NameAlt3`, `NameAlt4`,
     `Title`, `TitleAlt`, `CursorName`, `TypeFlags`, `TypeFlags2`, `Type`,
     `Family`, `Classification`, `KillCredit1`, `KillCredit2`, `VignetteID`,
     `Displayid1`, `Displayid2`, `Displayid3`, `Displayid4`,
     `HpMulti`, `PowerMulti`, `Leader`,
     `QuestItem1`, `QuestItem2`, `QuestItem3`, `QuestItem4`, `QuestItem5`,
     `QuestItem6`, `QuestItem7`, `QuestItem8`, `QuestItem9`, `QuestItem10`,
     `MovementInfoID`, `RequiredExpansion`, `FlagQuest`, `VerifiedBuild`)
VALUES
    (230007, 'Jarvis', '', '', '', '', '', '', '',
     'Experience Rate Master', '', '', 0, 0, 7,
     0, 0, 0, 0, 0,
     11049, 0, 0, 0,
     1, 1, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0);
