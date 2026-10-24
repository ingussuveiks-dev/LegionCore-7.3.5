START TRANSACTION;

SET @talonclaw_boarding :=
    EXISTS (SELECT 1 FROM creature_template WHERE entry=100965 AND AIName='SmartAI' AND ScriptName='' AND npcflag=16777216 AND npcflag2=0)
    AND (SELECT COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=100965)=1
    AND EXISTS (SELECT 1 FROM npc_spellclick_spells WHERE npc_entry=100965 AND spell_id=198941 AND cast_flags=1 AND user_type=0 AND add_npc_flag=1)
    AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=100965 AND source_type=0)=2
    AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=100965 AND source_type=0 AND Difficulties=''
        AND event_phase_mask=0 AND event_chance=100 AND event_flags=0 AND event_param1=0 AND event_param2=0
        AND event_param3=0 AND event_param4=0 AND event_param5=0 AND action_param2=0 AND action_param3=0
        AND action_param4=0 AND action_param5=0 AND action_param6=0 AND target_type=7
        AND target_param1=0 AND target_param2=0 AND target_param3=0 AND target_param4=0
        AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0
        AND ((id=0 AND link=1 AND event_type=73 AND action_type=33 AND action_param1=100111)
          OR (id=1 AND link=0 AND event_type=61 AND action_type=85 AND action_param1=198950)))=2
    AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=100965 AND SourceEntry=198941)
    AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=198941)
    AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=100965 AND SourceId=0)=1
    AND EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=100965 AND SourceId=0
        AND ElseGroup=0 AND ConditionTypeOrReference=9 AND ConditionTarget=0 AND ConditionValue1=40385
        AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=0 AND ErrorTextId=0 AND ScriptName='');

INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT scope.source_type,scope.source_group,scope.source_entry,0,0,rule.condition_type,rule.target,rule.value1,rule.value2,0,0,0,'','Talonclaw: admit only the living Dalaran Hunter claimant and boarding hippogryph'
FROM (SELECT 18 AS source_type,100965 AS source_group,198941 AS source_entry
      UNION ALL SELECT 17,0,198941
      UNION ALL SELECT 22,1,100965) scope
CROSS JOIN (SELECT 31 AS condition_type,0 AS target,4 AS value1,0 AS value2
      UNION ALL SELECT 15,0,4,0
      UNION ALL SELECT 9,0,40385,0
      UNION ALL SELECT 36,0,0,0
      UNION ALL SELECT 22,0,1220,0
      UNION ALL SELECT 31,1,3,100965
      UNION ALL SELECT 36,1,0,0
      UNION ALL SELECT 22,1,1220,0) rule
WHERE @talonclaw_boarding AND NOT(scope.source_type=22 AND rule.condition_type=9)
    AND (scope.source_type<>17 OR rule.target=0);

COMMIT;
