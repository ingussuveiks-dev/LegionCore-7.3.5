-- These three elevated rune spawns must not randomly choose the template's
-- invisible alternative11686. Preserve the native coordinates/animation kit.
UPDATE creature SET modelid=CASE id
 WHEN 106322 THEN 69560 WHEN 106323 THEN 69561 WHEN 106324 THEN 69562 END
WHERE map=1495 AND PhaseId=6985 AND modelid IN(0,11686)
 AND ((guid=382291 AND id=106322) OR (guid=382292 AND id=106323) OR (guid=382294 AND id=106324));

-- Flight2 means CanFly, not DisableGravity. Pin only the Hunter rune room's
-- three models and13 native lightning anchors; do not change the shared ship
-- template, corridor emitters or other class/scenario phases.
INSERT INTO creature_movement_override (SpawnId,Ground,Flight)
SELECT guid,0,1 FROM creature
WHERE map=1495 AND PhaseId=6985 AND
 ((id IN(106322,106323,106324) AND guid IN(382291,382292,382294))
 OR (id=64367 AND guid IN(382232,382234,382235,382240,382241,382242,
 382243,382246,382247,382248,382249,382250,382251)))
ON DUPLICATE KEY UPDATE Ground=VALUES(Ground),Flight=VALUES(Flight);

-- The highest native anchors are23 yards above Prustaga. They only passed
-- the old20-yard three-dimensional check after falling to the floor.
-- The existing1068/Stage3 condition still scopes this ritual update.
SET @titanstrike_rune_stage_gate = EXISTS (
 SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1
 AND SourceEntry=64367 AND SourceId=0 AND ElseGroup=0
 AND ConditionTypeOrReference=44 AND ConditionTarget=1
 AND ConditionValue1=1068 AND ConditionValue2=2 AND NegativeCondition=0);
UPDATE conditions SET ConditionValue2=30
WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=64367
 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=29
 AND ConditionTarget=1 AND ConditionValue1=104949 AND ConditionValue2=20
 AND ConditionValue3=0 AND NegativeCondition=0
 AND @titanstrike_rune_stage_gate;
