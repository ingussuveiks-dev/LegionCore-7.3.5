-- Actual EO157/13, not scene157/10. Gate the root and make cleanup a real
-- LINK so an independently triggered aura removal cannot bypass admission.
START TRANSACTION;
SET @hodir_landing_safe :=
 (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=157 AND source_type=13)=2
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=157 AND source_type=10)=0
 AND EXISTS(SELECT 1 FROM smart_scripts WHERE `entryorguid`=157 AND `source_type`=13 AND `id`=0 AND `link`=1 AND COALESCE(Difficulties,'')='' AND `event_type`=89 AND `event_phase_mask`=0 AND `event_chance`=100 AND `event_flags`=0 AND `event_param1`=0 AND `event_param2`=0 AND `event_param3`=0 AND `event_param4`=0 AND `event_param5`=0 AND `action_type`=205 AND `action_param1`=92 AND `action_param2`=52537 AND `action_param3`=0 AND `action_param4`=0 AND `action_param5`=0 AND `action_param6`=0 AND `target_type`=7 AND `target_param1`=0 AND `target_param2`=0 AND `target_param3`=0 AND `target_param4`=0 AND `target_x`=0 AND `target_y`=0 AND `target_z`=0 AND `target_o`=0)
 AND EXISTS(SELECT 1 FROM smart_scripts WHERE `entryorguid`=157 AND `source_type`=13 AND `id`=1 AND `link`=0 AND COALESCE(Difficulties,'')='' AND event_type IN(89,61) AND `event_phase_mask`=0 AND `event_chance`=100 AND `event_flags`=0 AND `event_param1`=0 AND `event_param2`=0 AND `event_param3`=0 AND `event_param4`=0 AND `event_param5`=0 AND `action_type`=28 AND `action_param1`=217815 AND `action_param2`=0 AND `action_param3`=0 AND `action_param4`=0 AND `action_param5`=0 AND `action_param6`=0 AND `target_type`=7 AND `target_param1`=0 AND `target_param2`=0 AND `target_param3`=0 AND `target_param4`=0 AND `target_x`=0 AND `target_y`=0 AND `target_z`=0 AND `target_o`=0)
 AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=157)=1
 AND EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=157 AND SourceId IN(10,13)
  AND ElseGroup=0 AND ConditionTypeOrReference=44 AND ConditionTarget=0 AND ConditionValue1=1146 AND ConditionValue2=2 AND ConditionValue3=0
  AND NegativeCondition=0 AND ErrorTextId=0 AND ScriptName='')
 AND (SELECT COUNT(*) FROM scenario_data WHERE ScenarioID=1146)=1
 AND EXISTS(SELECT 1 FROM scenario_data WHERE ScenarioID=1146 AND MapID=1489 AND Class=1 AND Team=0 AND DifficultyID=0 AND LfgDungeonID=0)
 AND (SELECT COUNT(*) FROM eventobject WHERE id=157)=1
 AND EXISTS(SELECT 1 FROM eventobject WHERE guid=400299 AND id=157 AND map=1489 AND spawnMask=4096 AND phaseMask=1 AND PhaseId=5896
  AND ABS(position_x-220.3)<0.001 AND ABS(position_y-1529.03)<0.001 AND ABS(position_z-(-30.4891))<0.001 AND ABS(orientation-2.80915)<0.001)
 AND EXISTS(SELECT 1 FROM eventobject_template WHERE entry=157 AND radius=40 AND SpellID=0 AND WorldSafeLocID=0 AND ScriptName='SmartEventObject' AND Flags=0);
SET @hodir_landing_guard=IF(COALESCE(@hodir_landing_safe,0),'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Hodir landing graph/route differ; review before updating''');
PREPARE hodir_landing_statement FROM @hodir_landing_guard;
EXECUTE hodir_landing_statement;
DEALLOCATE PREPARE hodir_landing_statement;
UPDATE conditions SET SourceId=13 WHERE @hodir_landing_safe AND SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=157 AND SourceId=10;
UPDATE smart_scripts SET event_type=61 WHERE @hodir_landing_safe AND entryorguid=157 AND source_type=13 AND id=1 AND event_type=89;
COMMIT;
