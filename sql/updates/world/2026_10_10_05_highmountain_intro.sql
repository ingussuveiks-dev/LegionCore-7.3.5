START TRANSACTION;
-- TDB 735.00 / build 25549 objective layout. The optional hidden Dalaran
-- objective still needs its existing exploration/event gate at Warbrave Oro.
UPDATE quest_objectives SET Flags=28 WHERE ID=279996 AND QuestID=39733 AND ObjectID=97377;
UPDATE quest_objectives SET Flags2=1 WHERE ID=279644 AND QuestID=39491 AND ObjectID=128397;
DELETE FROM quest_start_scripts WHERE id=39733 AND command=7 AND datalong=39733;
-- The original event chain remains responsible for the Dalaran arrival.
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 22,4,97666,0,0,36,0,0,0,0,0,0,'','The Lone Mountain - living player at Oro'
FROM DUAL WHERE NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=4 AND SourceEntry=97666 AND ConditionTypeOrReference=36);
-- Flight credit was granted before the player even spoke to the flightmaster.
-- Retain the linked native 198160 discovery of taxi node 1719.
UPDATE smart_scripts SET action_type=72,action_param1=0,comment='Keepers of the Hammer - close gossip; credit after actual taxi arrival'
WHERE entryorguid=97666 AND source_type=0 AND id=1 AND action_type=33 AND action_param1=96813;
INSERT INTO spell_script_names (spell_id,ScriptName)
SELECT 198160,'spell_highmountain_discover_thunder_totem' FROM DUAL
WHERE NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=198160 AND ScriptName='spell_highmountain_discover_thunder_totem');
DELETE FROM smart_scripts WHERE entryorguid=106244 AND source_type=0 AND id=0 AND action_type=80 AND action_param1=10624400;
DELETE FROM smart_scripts WHERE entryorguid=10624400 AND source_type=9;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=106244;
UPDATE creature_template SET AIName='',ScriptName='npc_highmountain_arrival_oro'
WHERE entry=106244 AND ScriptName IN('','npc_highmountain_arrival_oro');
DELETE FROM creature_questender WHERE id=93805 AND quest=38907;
-- Poison Idols: native POI objective 280574..280577 identifies these four
-- locations. Preserve the existing spawns, heights, appearance and phase.
UPDATE creature SET id=99434 WHERE guid=340567 AND id=99433 AND map=1220 AND ABS(position_x-4212.4)<1 AND ABS(position_y-4615.63)<1;
UPDATE creature SET id=99435 WHERE guid=340568 AND id=99433 AND map=1220 AND ABS(position_x-4259.53)<1 AND ABS(position_y-4634.77)<1;
UPDATE creature SET id=99436 WHERE guid=340569 AND id=99433 AND map=1220 AND ABS(position_x-4313.97)<1 AND ABS(position_y-4636.58)<1;
UPDATE creature_template SET AIName='',ScriptName='npc_highmountain_poison_idol'
WHERE entry IN(99433,99434,99435,99436) AND ScriptName IN('','npc_highmountain_poison_idol');
DELETE FROM smart_scripts WHERE source_type=0 AND entryorguid IN(99433,99434,99435,99436) AND id=0;
DELETE FROM smart_scripts WHERE source_type=9 AND entryorguid=9943300;
-- Native package 679 belongs to Ormgul, not also to Poisoned Crops.
UPDATE quest_template SET QuestPackageID=0 WHERE ID=39272 AND QuestPackageID=679;
-- Corrupt duplicate POI has impossible map/object IDs; the native (2,5) row
-- and its existing point remain. Do not change valid/custom POI rows.
DELETE FROM quest_poi WHERE QuestID=38907 AND BlobIndex=0 AND Idx1=5 AND MapID=402653184 AND QuestObjectiveID=738197504;
COMMIT;
