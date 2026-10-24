START TRANSACTION;

SET @stromkar_thoradin := EXISTS (SELECT 1 FROM creature_template WHERE entry=104276 AND AIName='SmartAI' AND ScriptName='npc_stromkar_zakajz')
AND EXISTS (SELECT 1 FROM creature_template WHERE entry=104307 AND AIName='SmartAI' AND ScriptName='' AND npcflag=0)
AND EXISTS (SELECT 1 FROM creature_template_wdb WHERE Entry=104307 AND Displayid1=68038 AND Displayid2=0 AND Displayid3=0 AND Displayid4=0)
AND NOT EXISTS (SELECT 1 FROM creature WHERE id=104307)
AND NOT EXISTS (SELECT 1 FROM creature_summon_groups WHERE entry=104307)
AND (SELECT COUNT(*) FROM waypoints WHERE entry=104307)=1
AND EXISTS (SELECT 1 FROM waypoints WHERE entry=104307 AND pointid=1 AND ABS(position_x-1886.91)<0.001 AND ABS(position_y-2194.33)<0.001 AND ABS(position_z-37.1166)<0.001)
AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=104276 AND source_type=0)=9
AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=104276 AND source_type=9)=4
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=104276 AND source_type=0 AND id=3 AND link=4 AND event_type=2 AND event_param1=0 AND event_param2=1 AND action_type=205 AND action_param1=92 AND action_param2=49108)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=104276 AND source_type=0 AND id=8 AND event_type=8 AND event_param1=201904 AND action_type=80 AND action_param1=104276 AND target_type=1)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=104276 AND source_type=9 AND id=1 AND action_type=45 AND action_param1=1 AND action_param2=1 AND target_type=19 AND target_param1=104307 AND target_param2=50)
AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=104307 AND source_type=0)=5
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=104307 AND source_type=0 AND id=0 AND link=2 AND event_type=54 AND action_type=1 AND action_param1=0 AND action_param2=6000 AND target_type=1)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=104307 AND source_type=0 AND id=2 AND event_type=61 AND action_type=53 AND action_param2=104307 AND target_type=1)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=104307 AND source_type=0 AND id=3 AND event_type=38 AND event_param1=1 AND event_param2=1 AND action_type=1 AND action_param1=2 AND action_param2=8000 AND target_type=1)
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=104276 AND SourceId=0 AND SourceGroup=10)
AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=206456)=16
AND (SELECT COUNT(*) FROM conditions current
    JOIN (SELECT 0 AS else_group,9 AS quest_type UNION ALL SELECT 1,28) AS variant ON current.ElseGroup=variant.else_group
    JOIN (SELECT 15 AS condition_type,1 AS value1,0 AS value2,0 AS negative
        UNION ALL SELECT 22,1539,0,0
        UNION ALL SELECT 44,1037,9,0
        UNION ALL SELECT 0,41105,0,0
        UNION ALL SELECT 8,41105,0,1
        UNION ALL SELECT 3,128910,0,0
        UNION ALL SELECT 36,0,0,0
        UNION ALL SELECT 1,206448,0,0) AS rule
        ON current.ConditionTypeOrReference=IF(rule.condition_type=0,variant.quest_type,rule.condition_type)
        AND current.ConditionValue1=rule.value1 AND current.ConditionValue2=rule.value2 AND current.NegativeCondition=rule.negative
    WHERE current.SourceTypeOrReferenceId=17 AND current.SourceEntry=206456 AND current.SourceGroup=0 AND current.SourceId=0
        AND current.ConditionTarget=0 AND current.ConditionValue3=0 AND current.ErrorTextId=0 AND current.ScriptName='')=16
AND (SELECT COUNT(*) FROM spell_linked_spell WHERE spell_trigger=206456)=1
AND EXISTS (SELECT 1 FROM spell_linked_spell WHERE spell_trigger=206456 AND spell_effect=201904 AND type=0 AND caster=0 AND target=0 AND actiontype=0 AND duration=0
    AND hastype=0 AND hastalent=0 AND hasparam=0 AND hastype2=0 AND hastalent2=0 AND hasparam2=0 AND chance=0 AND cooldown=0
    AND hitmask=0 AND removeMask=0 AND effectMask=0 AND targetCountType=0 AND targetCount=-1 AND `group`=0 AND param=0 AND randList='');

INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,
    action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_x,target_y,target_z,target_o,comment)
SELECT 104276,0,9,0,60,0,100,0,0,0,500,500,12,104307,8,0,0,0,0,8,0,0,0,position_x,position_y,position_z,0,
    'Stromkar: retry missing final Thoradin at existing waypoint only after defeat'
FROM waypoints WHERE entry=104307 AND pointid=1 AND @stromkar_thoradin;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 22,10,104276,0,0,rule.condition_type,1,rule.value1,rule.value2,0,rule.negative,0,'',
    'Stromkar: final Thoradin only for live defeated final-stage Zakajz without a nearby receiver'
FROM (SELECT 31 AS condition_type,3 AS value1,104276 AS value2,0 AS negative
    UNION ALL SELECT 22,1539,0,0
    UNION ALL SELECT 44,1037,9,0
    UNION ALL SELECT 36,0,0,0
    UNION ALL SELECT 38,1,4,0
    UNION ALL SELECT 29,104307,50,1) AS rule
WHERE @stromkar_thoradin;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 17,0,206456,0,variant.else_group,29,0,104307,50,0,0,0,'',
    'Stromkar: retain Execute retry until its actual final-dialogue receiver exists'
FROM (SELECT 0 AS else_group UNION ALL SELECT 1) AS variant
WHERE @stromkar_thoradin;

COMMIT;
