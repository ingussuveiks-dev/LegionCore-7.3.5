-- Jagganoth's arrival (200011 effect 0) uses an AREA_ENTRY target, without
-- implicit target conditions in the original data. This also knocks away
-- the Summoning Portal NPC and Zelifrax, who must remain free to challenge him.
-- Limit the Dalaran ritual to its four council members and participating
-- players. Preserve the existing targeting in the later map-1522 escape.
-- Condition target 0 = candidate, target 1 = caster. ElseGroup 0 is the
-- non-Dalaran branch; its two negative map conditions are ANDed.
INSERT IGNORE INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
VALUES
 (13,1,200011,0,0,22,1,1220,0,0,1,'New Blood knockback - preserve non-Dalaran scenes'),
 (13,1,200011,0,0,22,1,1502,0,0,1,'New Blood knockback - preserve non-Underbelly scenes'),
 (13,1,200011,0,1,31,0,4,0,0,0,'New Blood knockback - player'),
 (13,1,200011,0,1,9,0,40729,0,0,0,'New Blood knockback - active ritual participant'),
 (13,1,200011,0,2,31,0,3,101456,0,0,'New Blood knockback - Ritssyn'),
 (13,1,200011,0,3,31,0,3,101454,0,0,'New Blood knockback - Zinnin'),
 (13,1,200011,0,4,31,0,3,101455,0,0,'New Blood knockback - Shinfel'),
 (13,1,200011,0,5,31,0,3,101457,0,0,'New Blood knockback - Jubeka');

-- SetScript9 forbids replacing a timed list from inside that same list.
-- Inline the existing 10145601 recovery tail instead of calling it on self.
-- Preserve its 120s + 1s delay, evade/home action, 5s pause and restored flags.
-- Keep 10145601 itself intact. Match only the known broken call site.
UPDATE smart_scripts SET event_param1=121000,event_param2=121000,
 action_type=47,action_param1=0,action_param2=0,
 comment='New Blood - inline recovery - hide Ritssyn'
WHERE entryorguid=10145600 AND source_type=9 AND id=3 AND link=0
 AND event_param1=120000 AND event_param2=120000
 AND action_type=80 AND action_param1=10145601 AND action_param2=2 AND target_type=1;

INSERT IGNORE INTO smart_scripts
 (entryorguid,source_type,id,event_type,event_chance,event_param1,event_param2,
  action_type,action_param1,target_type,comment)
SELECT 10145600,9,tail.id+3,0,100,tail.event_param1,tail.event_param2,
 tail.action_type,tail.action_param1,1,'New Blood - inline Ritssyn recovery tail'
FROM smart_scripts tail
WHERE tail.entryorguid=10145601 AND tail.source_type=9 AND tail.id BETWEEN 1 AND 3 AND tail.link=0
 AND EXISTS (SELECT 1 FROM smart_scripts head WHERE head.entryorguid=10145600
  AND head.source_type=9 AND head.id=3 AND head.link=0 AND head.action_type=47
  AND head.action_param1=0 AND head.event_param1=121000
  AND head.comment='New Blood - inline recovery - hide Ritssyn');
