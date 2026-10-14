-- Private Trial of Will defense, explicit record access and completed recording.
UPDATE `gameobject_template` SET `ScriptName`='go_stormheim_will_console'
WHERE `entry` IN (243814,243802) AND `ScriptName` IN ('','go_stormheim_will_console');
UPDATE `creature_template` SET `ScriptName`='npc_stormheim_will_actor'
WHERE `entry` IN (96139,96144,96129,108263) AND `ScriptName` IN ('','npc_stormheim_will_actor');
-- Original TDB735 build-25549 target, preserving the persisted objective slot.
UPDATE `quest_objectives` SET `Type`=2, `ObjectID`=243802
WHERE `ID`=279721 AND `QuestID`=39592 AND `StorageIndex`=3 AND `Amount`=1
  AND `Type`=0 AND `ObjectID`=96144;
-- Four duplicated zero-delay placeholders completed the quest on acceptance.
-- They also made script loading restore the exploration flag. Preserve any
-- custom start actions and detach the hook only when its list becomes empty.
DELETE FROM `quest_start_scripts`
WHERE `id`=39592 AND `delay`=0 AND `command`=7 AND `datalong`=39592
  AND `datalong2`=0 AND `dataint`=0 AND `x`=0 AND `y`=0 AND `z`=0 AND `o`=0;
UPDATE `quest_template` SET `StartScript`=0
WHERE `ID`=39592 AND `StartScript`=39592
  AND NOT EXISTS (SELECT 1 FROM `quest_start_scripts` WHERE `id`=39592);
-- This archive is completed by the restored GO objective. Keep a custom
-- exploration action's flag if one remains; clear only the known flag-2 state.
UPDATE `quest_template_addon` SET `SpecialFlags`=`SpecialFlags` & ~2
WHERE `ID`=39592 AND `SpecialFlags`=2
  AND NOT EXISTS (SELECT 1 FROM `quest_start_scripts` WHERE `command`=7 AND `datalong`=39592);
