START TRANSACTION;

SET @marksman_return_native := EXISTS (
    SELECT 1 FROM creature_template AS actor
    JOIN smart_scripts AS option_event ON option_event.entryorguid=actor.entry AND option_event.source_type=0
    JOIN smart_scripts AS flag_event ON flag_event.entryorguid=actor.entry AND flag_event.source_type=0 AND flag_event.id=55
    JOIN smart_scripts AS travel_event ON travel_event.entryorguid=actor.entry AND travel_event.source_type=0 AND travel_event.id=56
    JOIN gossip_menu_option AS menu ON menu.MenuID=19331 AND menu.OptionID=0
    WHERE actor.entry=100397 AND actor.AIName='SmartAI' AND actor.ScriptName='' AND actor.gossip_menu_id=19331
      AND option_event.id IN(55,59) AND option_event.event_type=62
      AND option_event.event_param1=19331 AND option_event.event_param2=0 AND option_event.link=56
      AND option_event.action_type=33 AND option_event.action_param1=103481 AND option_event.target_type=7
      AND flag_event.event_type=61 AND flag_event.link=0 AND flag_event.action_type=81
      AND flag_event.action_param1=1 AND flag_event.target_type=1
      AND travel_event.event_type=61 AND travel_event.link=0 AND travel_event.action_type=85
      AND travel_event.action_param1=205040 AND travel_event.target_type=7
      AND menu.OptionBroadcastTextID=77067 AND menu.ActionMenuID=0 AND menu.ActionPoiID=0
) AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id IN(55,59))=2
AND NOT EXISTS (
    SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=59
      AND NOT(event_type=62 AND event_param1=19331 AND event_param2=0 AND link=56
              AND action_type=33 AND action_param1=103481 AND target_type=7)
) AND NOT EXISTS (
    SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=56 AND SourceEntry=100397 AND SourceId=0
)
AND NOT EXISTS (
    SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND link=59
);

UPDATE smart_scripts SET id=59
WHERE entryorguid=100397 AND source_type=0 AND id=55 AND event_type=62
  AND event_param1=19331 AND event_param2=0 AND link=56 AND action_type=33
  AND action_param1=103481 AND target_type=7 AND @marksman_return_native
  AND NOT EXISTS (SELECT 1 FROM (SELECT id FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=59) AS occupied);

UPDATE smart_scripts SET event_flags=event_flags & ~1
WHERE entryorguid=100397 AND source_type=0 AND id=59 AND event_type=62
  AND event_param1=19331 AND event_param2=0 AND link=56 AND action_type=33
  AND action_param1=103481 AND target_type=7 AND @marksman_return_native;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT scope.source_type,scope.source_group,scope.source_entry,0,variant.else_group,
       IF(rule.condition_type=0,variant.quest_type,rule.condition_type),0,rule.value1,
       CASE rule.condition_type WHEN 44 THEN 8 WHEN 2 THEN 1 ELSE 0 END,
       IF(rule.condition_type=2,1,0),rule.negative,0,'','Marksmanship: completed native bow acquisition before Vereesa return'
FROM (
    SELECT 15 AS source_type,19331 AS source_group,0 AS source_entry
    UNION ALL SELECT 22,60,100397
) AS scope
CROSS JOIN (
    SELECT 0 AS else_group,9 AS quest_type
    UNION ALL SELECT 1,28
) AS variant
CROSS JOIN (
    SELECT 2 AS condition_type,128826 AS value1,0 AS negative
    UNION ALL SELECT 15,4,0
    UNION ALL SELECT 22,1489,0
    UNION ALL SELECT 44,972,0
    UNION ALL SELECT 0,40419,0
    UNION ALL SELECT 8,40419,1
) AS rule
WHERE @marksman_return_native
  AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=59
              AND event_type=62 AND event_param1=19331 AND event_param2=0 AND link=56
              AND action_type=33 AND action_param1=103481 AND target_type=7)
  AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=scope.source_type
                  AND SourceGroup=scope.source_group AND SourceEntry=scope.source_entry AND SourceId=0);

COMMIT;
