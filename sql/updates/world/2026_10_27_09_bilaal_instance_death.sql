-- Both735 Bilaal true-death handoffs now belong to instance1583 CreatureDies.
-- CODE-COUPLED: promote the matching instance handler before exposing this file to updater.
-- Native combat0..4, quest kill rewards and earlier Azuregos creators remain unchanged.
-- The complete graph exceeds the MySQL default 1024-byte aggregation limit.
SET @bilaal_death_previous_group_concat := @@SESSION.group_concat_max_len;
SET SESSION group_concat_max_len=GREATEST(@@SESSION.group_concat_max_len,1048576);
START TRANSACTION;
SET @bilaal_death_graph := (SELECT SHA2(GROUP_CONCAT(CONCAT_WS('|',entryorguid,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment) ORDER BY entryorguid,id SEPARATOR '\n'),256)
 FROM smart_scripts WHERE entryorguid IN(104502,106708) AND source_type=0);
SET @bilaal_death_count := (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid IN(104502,106708) AND source_type=0);
SET @bilaal_death_safe := ((@bilaal_death_count=14 AND @bilaal_death_graph='a306ccf1aa3ff9a3e92fb2ea23df4f2d282a7a34a039e484947f041312171a57')
 OR(@bilaal_death_count=10 AND @bilaal_death_graph='c7162d84ebc8a7f4fd2cc65690dac77cb35518c5bf40981bac3a165a873c1c49'))
 AND (SELECT COUNT(*) FROM creature_template WHERE entry IN(104502,106708,104546) AND AIName='SmartAI' AND ScriptName='')=3
 AND (SELECT COUNT(*) FROM instance_template WHERE map=1583 AND parent=0 AND script='instance_priest_mage_nexus_art' AND allowMount=1 AND bonusChance=20)=1
 AND (SELECT COUNT(*) FROM creature WHERE(guid=370522 AND id=104502 AND map=1583 AND PhaseId='6140' AND phaseMask=1)
 OR(guid=370551 AND id=106708 AND map=1583 AND PhaseId='6068' AND phaseMask=1))=2
 AND (SELECT COUNT(*) FROM creature WHERE id IN(104502,106708))=2
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE source_type=0 AND entryorguid IN(-370522,-370551))
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE action_type=12 AND action_param1 IN(104502,106708))
 AND NOT EXISTS(SELECT 1 FROM creature_summon_groups WHERE entry IN(104502,106708))
 AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry IN(104502,106708,-370522,-370551));
SET @bilaal_death_guard := IF(@bilaal_death_safe,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Bilaal native death graph/instance/source conditions differ; review before promotion''');
PREPARE bilaal_death_check FROM @bilaal_death_guard;
EXECUTE bilaal_death_check;
DEALLOCATE PREPARE bilaal_death_check;
DELETE FROM smart_scripts WHERE @bilaal_death_safe AND entryorguid IN(104502,106708) AND source_type=0 AND id IN(5,6);
COMMIT;
SET SESSION group_concat_max_len=@bilaal_death_previous_group_concat;
