-- The four entry roots dispatch as EventObject SmartScripts.
UPDATE conditions SET SourceId=13
WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceId=10
  AND SourceEntry IN (197,367,501,533);

-- Phase 7515 is shared. Only the active Black Rook Hold quest owns this line.
INSERT INTO conditions
(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,
 ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment)
VALUES (22,1,367,13,0,9,0,44789,'EO367 requires active 44789')
ON DUPLICATE KEY UPDATE Comment=VALUES(Comment);

-- The delivery aura supplies a temporary button to use on the nearby receiver.
-- It stays available across the short walk and ends with the quest or zone.
INSERT INTO spell_area
(spell,area,quest_start,quest_end,aura_spell,racemask,classmask,active_event,
 gender,autocast,quest_start_status,quest_end_status)
VALUES (206158,7502,41397,0,0,0,0,0,2,0,8,11)
ON DUPLICATE KEY UPDATE quest_start_status=VALUES(quest_start_status),autocast=VALUES(autocast);

-- A still active quest can already have the stone delivery objective.
INSERT INTO conditions
(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,
 ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,
 ConditionValue3,NegativeCondition,Comment)
VALUES (22,1,533,13,0,41,0,41397,104180,1,1,'EO533 requires undelivered stone')
ON DUPLICATE KEY UPDATE Comment=VALUES(Comment);
