-- Retain the existing spell-click and dialogue. Fire the dialogue after the
-- native flame creation and credit, and bind the Edict's native burn spell.
START TRANSACTION;

SET @burning_scroll_safe :=
    EXISTS (SELECT 1 FROM `creature_template` WHERE `entry`=53566 AND `AIName`='SmartAI' AND `ScriptName`='')
    AND EXISTS (SELECT 1 FROM `smart_scripts` WHERE `entryorguid`=53566 AND `source_type`=0 AND `id`=0
        AND `link`=1 AND `event_type`=64 AND `action_type`=85 AND `action_param1`=114611 AND `target_type`=7)
    AND EXISTS (SELECT 1 FROM `smart_scripts` WHERE `entryorguid`=53566 AND `source_type`=0 AND `id`=1
        AND `link`=0 AND `event_type`=61 AND `action_type`=1 AND `action_param1`=0 AND `target_type`=1)
    AND EXISTS (SELECT 1 FROM `gameobject_template` WHERE `entry`=210986 AND `type`=10
        AND `Data1`=29408 AND `Data10`=114580 AND `ScriptName`='')
    AND EXISTS (SELECT 1 FROM `gameobject` WHERE `guid`=102466 AND `id`=210986 AND `map`=860 AND `areaId`=5834)
    AND EXISTS (SELECT 1 FROM `npc_spellclick_spells` WHERE `npc_entry`=53566 AND `spell_id`=114746);

UPDATE `creature_template` SET `ScriptName`='npc_wandering_isle_scroll_master'
WHERE `entry`=53566 AND @burning_scroll_safe;

UPDATE `gameobject_template` SET `ScriptName`='go_wandering_isle_edict'
WHERE `entry`=210986 AND @burning_scroll_safe;

UPDATE `smart_scripts`
SET `link`=0,`event_type`=38,`event_param1`=29408,`event_param2`=1,
    `action_type`=1,`action_param1`=0,`target_type`=1,
    `comment`='Master Shang Xi - After snatching the flame - Talk'
WHERE `entryorguid`=53566 AND `source_type`=0 AND `id`=0 AND @burning_scroll_safe;

DELETE FROM `smart_scripts` WHERE `entryorguid`=53566 AND `source_type`=0 AND `id`=1
  AND @burning_scroll_safe AND NOT EXISTS (
      SELECT 1 FROM (SELECT `link` FROM `smart_scripts` WHERE `entryorguid`=53566 AND `source_type`=0) AS incoming
      WHERE incoming.`link`=1);

INSERT IGNORE INTO `spell_script_names` (`spell_id`,`ScriptName`)
SELECT 114580,'spell_wandering_isle_burn_scroll' WHERE @burning_scroll_safe;

INSERT IGNORE INTO `conditions`
    (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,
     `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
SELECT 13,15,114580,0,0,31,0,4,0,0,0,0,'','Burning Scroll - native player area targets' WHERE @burning_scroll_safe
UNION ALL
SELECT 18,53566,114746,0,0,23,1,5834,0,0,0,0,'','Snatch the flame - Shang Xi in the training grounds' WHERE @burning_scroll_safe;

UPDATE `quest_template` SET `ItemDrop1`=80212,`ItemDropQuantity1`=1
WHERE `ID`=29408 AND `ItemDrop1`=0 AND `ItemDropQuantity1`=0 AND @burning_scroll_safe;

UPDATE `spell_area` SET `quest_start`=29408,`quest_start_status`=8
WHERE `spell`=114610 AND `area`=5834 AND `quest_start`=0 AND `quest_end`=0 AND `autocast`=0
  AND @burning_scroll_safe;

COMMIT;
