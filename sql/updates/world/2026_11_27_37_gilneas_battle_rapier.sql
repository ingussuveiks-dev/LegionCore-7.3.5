-- Rapier of the Gilnean Patriots: the item is used during quest 24904.
CREATE TEMPORARY TABLE `gilneas_rapier_targets` (`group_id` tinyint unsigned PRIMARY KEY, `entry` mediumint unsigned NOT NULL);
INSERT INTO `gilneas_rapier_targets` VALUES
    (0,38221),(1,38425),(2,38348),(3,38218),(4,38465),(5,38466),(6,38426),(7,38415),(8,38470),(9,38474);

CREATE TEMPORARY TABLE `gilneas_rapier_conditions` LIKE `conditions`;
INSERT INTO `gilneas_rapier_conditions`
    (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`ElseGroup`,`ConditionTypeOrReference`,
     `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`Comment`)
SELECT 13,3,71388,`group_id`,31,0,3,`entry`,'Gilnean Patriots Rapier - battle ally for both effects'
FROM `gilneas_rapier_targets`;
INSERT INTO `gilneas_rapier_conditions`
    (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`ElseGroup`,`ConditionTypeOrReference`,
     `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`Comment`)
SELECT 13,3,71388,`group_id`,34,0,1,248,'Gilnean Patriots Rapier - allow neutral or friendly battle allies'
FROM `gilneas_rapier_targets`;
INSERT INTO `gilneas_rapier_conditions`
    (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`ElseGroup`,`ConditionTypeOrReference`,`Comment`)
SELECT 13,3,71388,`group_id`,36,'Gilnean Patriots Rapier - living battle allies only'
FROM `gilneas_rapier_targets`;

START TRANSACTION;

SET @gilneas_rapier_safe :=
    EXISTS (SELECT 1 FROM `quest_template` WHERE `ID`=24904 AND `StartItem`=50334)
    AND EXISTS (SELECT 1 FROM `quest_template_addon` WHERE `ID`=24904 AND `ProvidedItemCount`=1 AND `PrevQuestID`=24676)
    AND EXISTS (SELECT 1 FROM `spell_area` WHERE `spell`=71388 AND `area`=4842 AND `quest_start`=24904)
    AND NOT EXISTS (SELECT 1 FROM `spell_area` WHERE `spell`=71388 AND NOT
        (`aura_spell`=0 AND `racemask`=0 AND `classmask`=0 AND `active_event`=0 AND `gender`=2 AND `autocast`=0 AND `quest_start`=24904
            AND ((`area`=4842 AND `quest_start_status`=64 AND `quest_end`=24904 AND `quest_end_status`=11)
                OR (`area` IN(4842,4755) AND `quest_start_status`=8 AND `quest_end`=0 AND `quest_end_status`=0))))
    AND NOT EXISTS (SELECT 1 FROM `spell_script_names` WHERE `spell_id`=71388 AND `ScriptName`<>'spell_gilneas_patriots_rapier')
    AND NOT EXISTS (SELECT 1 FROM `conditions` c LEFT JOIN `gilneas_rapier_conditions` e
        USING (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,
            `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`)
        WHERE c.`SourceTypeOrReferenceId` IN(13,17) AND c.`SourceEntry`=71388
            AND (e.`SourceEntry` IS NULL OR c.`NegativeCondition`<>0 OR c.`ErrorTextId`<>0 OR c.`ScriptName`<>''));

SET @gilneas_rapier_guard := IF(@gilneas_rapier_safe, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Gilnean Patriots Rapier: active quest or implicit target context differs''');
PREPARE gilneas_rapier_check FROM @gilneas_rapier_guard;
EXECUTE gilneas_rapier_check;
DEALLOCATE PREPARE gilneas_rapier_check;

UPDATE `spell_area` SET `quest_start_status`=8,`quest_end`=0,`quest_end_status`=0
WHERE @gilneas_rapier_safe AND `spell`=71388 AND `area`=4842 AND `quest_start`=24904 AND `quest_end`=24904;

INSERT INTO `spell_area`
    (`spell`,`area`,`quest_start`,`quest_end`,`aura_spell`,`racemask`,`classmask`,`active_event`,`gender`,`autocast`,`quest_start_status`,`quest_end_status`)
SELECT 71388,4755,24904,0,0,0,0,0,2,0,8,0
WHERE @gilneas_rapier_safe AND NOT EXISTS (SELECT 1 FROM `spell_area` WHERE `spell`=71388 AND `area`=4755);

INSERT INTO `conditions` SELECT e.* FROM `gilneas_rapier_conditions` e
WHERE @gilneas_rapier_safe AND NOT EXISTS (SELECT 1 FROM `conditions` c WHERE
    (c.`SourceTypeOrReferenceId`,c.`SourceGroup`,c.`SourceEntry`,c.`SourceId`,c.`ElseGroup`,c.`ConditionTypeOrReference`,
     c.`ConditionTarget`,c.`ConditionValue1`,c.`ConditionValue2`,c.`ConditionValue3`) =
    (e.`SourceTypeOrReferenceId`,e.`SourceGroup`,e.`SourceEntry`,e.`SourceId`,e.`ElseGroup`,e.`ConditionTypeOrReference`,
     e.`ConditionTarget`,e.`ConditionValue1`,e.`ConditionValue2`,e.`ConditionValue3`));

INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`)
SELECT 71388,'spell_gilneas_patriots_rapier' WHERE @gilneas_rapier_safe
AND NOT EXISTS (SELECT 1 FROM `spell_script_names` WHERE `spell_id`=71388 AND `ScriptName`='spell_gilneas_patriots_rapier');

COMMIT;
DROP TEMPORARY TABLE `gilneas_rapier_conditions`, `gilneas_rapier_targets`;
