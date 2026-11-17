-- The Krasus' Landing hippogryph is transport for Call of the Marksman.
-- Its SmartAI credit/teleport already requires40392; gate the spellclick
-- itself too, so an ineligible viewer cannot click/cast201568.
SET @hunter_hippogryph_safe := (
    (SELECT COUNT(*) FROM `npc_spellclick_spells` WHERE `npc_entry`=102177) = 1
    AND (SELECT COUNT(*) FROM `npc_spellclick_spells` WHERE `npc_entry`=102177
        AND `spell_id`=201568 AND `cast_flags`=1 AND `user_type`=0 AND `add_npc_flag`=1) = 1
    AND (SELECT COUNT(*) FROM `creature_template` WHERE `entry`=102177
        AND `AIName`='SmartAI' AND `ScriptName`='') = 1
    AND (SELECT COUNT(*) FROM `quest_objectives` WHERE `QuestID`=40392
        AND `Type`=0 AND `StorageIndex`=0 AND `ObjectID`=102173 AND `Amount`=1) = 1
    AND EXISTS (SELECT 1 FROM `conditions` WHERE `SourceTypeOrReferenceId`=22
        AND `SourceGroup`=1 AND `SourceEntry`=102177 AND `ElseGroup`=0
        AND `ConditionTypeOrReference`=9 AND `ConditionTarget`=0
        AND `ConditionValue1`=40392 AND `NegativeCondition`=0)
    AND NOT EXISTS (
        SELECT 1 FROM `conditions` WHERE `SourceTypeOrReferenceId`=18 AND `SourceGroup`=102177
        AND NOT (`SourceEntry`=201568 AND `SourceId`=0 AND `ElseGroup`=0
            AND `ConditionTypeOrReference`=9 AND `ConditionTarget`=0
            AND `ConditionValue1`=40392 AND `ConditionValue2`=0 AND `ConditionValue3`=0
            AND `NegativeCondition`=0 AND `ErrorTextId`=0 AND `ScriptName`=''
            AND COALESCE(`Comment`,'')='Hunter marksman hippogryph - active Call of the Marksman only')
    )
);
SET @hunter_hippogryph_guard := IF(@hunter_hippogryph_safe, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Hunter marksman hippogryph native context differs''');
PREPARE hunter_hippogryph_check FROM @hunter_hippogryph_guard;
EXECUTE hunter_hippogryph_check;
DEALLOCATE PREPARE hunter_hippogryph_check;

INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,
    `ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,
    `NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
VALUES (18,102177,201568,0,0,9,0,40392,0,0,0,0,'','Hunter marksman hippogryph - active Call of the Marksman only')
ON DUPLICATE KEY UPDATE `Comment`='Hunter marksman hippogryph - active Call of the Marksman only';

SET @hunter_hippogryph_safe := NULL;
SET @hunter_hippogryph_guard := NULL;
