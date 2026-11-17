-- Visage of Ohn'ahra is the quest-only Oath of Service interaction.
-- Keep its native203156 cast/credit/summon; filter admission per viewer.
SET @hunter_oath_safe := (
    (SELECT COUNT(*) FROM `npc_spellclick_spells` WHERE `npc_entry`=102694) = 1
    AND (SELECT COUNT(*) FROM `npc_spellclick_spells` WHERE `npc_entry`=102694
        AND `spell_id`=203156 AND `cast_flags`=1 AND `user_type`=0 AND `add_npc_flag`=1) = 1
    AND (SELECT COUNT(*) FROM `creature_template` WHERE `entry`=102694
        AND `AIName`='' AND `ScriptName`='') = 1
    AND (SELECT COUNT(*) FROM `quest_objectives` WHERE `QuestID`=40955
        AND `Type`=0 AND `StorageIndex`=0 AND `ObjectID`=102794 AND `Amount`=1) = 1
    AND NOT EXISTS (
        SELECT 1 FROM `conditions` WHERE `SourceTypeOrReferenceId`=18 AND `SourceGroup`=102694
        AND NOT (`SourceEntry`=203156 AND `SourceId`=0 AND `ElseGroup`=0
            AND `ConditionTypeOrReference`=9 AND `ConditionTarget`=0
            AND `ConditionValue1`=40955 AND `ConditionValue2`=0 AND `ConditionValue3`=0
            AND `NegativeCondition`=0 AND `ErrorTextId`=0 AND `ScriptName`=''
            AND COALESCE(`Comment`,'')='Hunter oath statue - active Oath of Service only')
    )
);
SET @hunter_oath_guard := IF(@hunter_oath_safe, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Hunter oath statue native context differs''');
PREPARE hunter_oath_check FROM @hunter_oath_guard;
EXECUTE hunter_oath_check;
DEALLOCATE PREPARE hunter_oath_check;

INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,
    `ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,
    `NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
VALUES (18,102694,203156,0,0,9,0,40955,0,0,0,0,'','Hunter oath statue - active Oath of Service only')
ON DUPLICATE KEY UPDATE `Comment`='Hunter oath statue - active Oath of Service only';

SET @hunter_oath_safe := NULL;
SET @hunter_oath_guard := NULL;
