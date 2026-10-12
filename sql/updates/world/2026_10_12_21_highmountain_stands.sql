-- Native TDB735 requires one confrontation, not an automatic quest completion.
UPDATE `quest_objectives` SET `Amount`=1
WHERE `ID`=284367 AND `QuestID`=42512 AND `Type`=0 AND `ObjectID`=107704
  AND `StorageIndex`=0 AND `Amount`=0 AND `Flags`=1;

-- Start package 1638 at the native confrontation POI, not at Mayla's quest acceptance.
-- Preserve Mayla's unrelated proximity credit and The Underking achievement event.
DELETE FROM `smart_scripts` WHERE `entryorguid`=93846 AND `source_type`=0 AND `id`=2
  AND `event_type`=19 AND `event_param1`=42512 AND `action_type`=85 AND `action_param1`=213725;

UPDATE `spell_scene` SET `ScriptName`='scene_highmountain_stands'
WHERE `MiscValue`=1287 AND `SceneScriptPackageID`=1638
  AND (`ScriptName` IS NULL OR `ScriptName` IN ('','scene_highmountain_stands'));
