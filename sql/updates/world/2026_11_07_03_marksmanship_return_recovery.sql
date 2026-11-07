-- Code-coupled: retain outside live updater until the matching Release is ready.
START TRANSACTION;
SET @mm_recovery_safe :=
 (SELECT COUNT(*) FROM creature_template WHERE entry=100397 AND AIName='SmartAI' AND ScriptName='' AND gossip_menu_id=19331)=1
 AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=100397 AND SourceId=0)=2
 AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=100397 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=32 AND ConditionTarget=0 AND ConditionValue1=16 AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=0 AND ErrorTextId=0 AND ScriptName IN('','condition_marksmanship_recovered_arrival'))=1
 AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=100397 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=44 AND ConditionTarget=1 AND ConditionValue1=972 AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=0 AND ScriptName='')=1
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=0 AND link=1 AND event_type=10 AND event_param1=1 AND event_param2=10 AND action_type=1 AND action_param1=0 AND target_type=1)=1
 AND (SELECT COUNT(*) FROM spell_script_names WHERE spell_id=205040 AND ScriptName='spell_marksmanship_return')=1
 AND (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=40419)=6
 AND (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=40419 AND Type=0 AND Amount=1 AND ((ObjectID=102128 AND StorageIndex=0) OR (ObjectID=102129 AND StorageIndex=1) OR (ObjectID=102131 AND StorageIndex=2) OR (ObjectID=102548 AND StorageIndex=3) OR (ObjectID=103481 AND StorageIndex=4) OR (ObjectID=103490 AND StorageIndex=5)))=6;
SET @mm_recovery_guard=IF(COALESCE(@mm_recovery_safe,0),'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Marksmanship recovery native graph differs; review before updating''');
PREPARE mm_recovery_statement FROM @mm_recovery_guard;
EXECUTE mm_recovery_statement;
DEALLOCATE PREPARE mm_recovery_statement;
UPDATE conditions SET ScriptName='condition_marksmanship_recovered_arrival'
WHERE @mm_recovery_safe AND SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=100397 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=32 AND ConditionTarget=0 AND ConditionValue1=16 AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=0 AND ScriptName='';
COMMIT;
