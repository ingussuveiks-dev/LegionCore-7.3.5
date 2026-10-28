START TRANSACTION;
SET @stromkar_awake :=
 EXISTS(SELECT 1 FROM creature_template WHERE entry=103398 AND AIName='SmartAI' AND ScriptName='')
 AND EXISTS(SELECT 1 FROM creature_template WHERE entry=104276 AND AIName='SmartAI' AND ScriptName='npc_stromkar_zakajz')
 AND EXISTS(SELECT 1 FROM creature WHERE guid=370991 AND id=103398 AND map=1539 AND PhaseId='5739')
 AND (SELECT COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=103398)=1
 AND EXISTS(SELECT 1 FROM npc_spellclick_spells WHERE npc_entry=103398 AND spell_id=204771 AND cast_flags=1 AND user_type=0 AND add_npc_flag=1)
 AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=103398)
 AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(206360,206361))
 AND NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=206360)
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=103398)=5
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=103398 AND Difficulties='' AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
  AND event_param3=0 AND event_param4=0 AND event_param5=0 AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
  AND target_param1=0 AND target_param2=0 AND target_param3=0 AND target_param4=0 AND (
   (source_type=0 AND id=0 AND link=1 AND event_type=73 AND event_param1=0 AND event_param2=0 AND action_type=80 AND action_param1=103398 AND action_param2=2 AND target_type=1 AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0)
   OR(source_type=0 AND id=1 AND link=2 AND event_type=61 AND event_param1=0 AND event_param2=0 AND action_type=205 AND action_param1=92 AND action_param2=49101 AND target_type=7 AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0)
   OR(source_type=0 AND id=2 AND link=0 AND event_type=61 AND event_param1=0 AND event_param2=0 AND action_type=81 AND action_param1=0 AND action_param2=0 AND target_type=1 AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0)
   OR(source_type=9 AND id=0 AND link=0 AND event_type=0 AND event_param1=5000 AND event_param2=5000 AND action_type=12 AND action_param1=104276 AND action_param2=7 AND target_type=8 AND ABS(target_x-1882.18)<0.01 AND ABS(target_y-2163.35)<0.01 AND ABS(target_z-35.9247)<0.01 AND ABS(target_o-2.00205)<0.001)
   OR(source_type=9 AND id=1 AND link=0 AND event_type=0 AND event_param1=0 AND event_param2=0 AND action_type=41 AND action_param1=0 AND action_param2=0 AND target_type=1 AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0)))=5;
-- The registered controller replaces this complete five-row opening graph:
-- real blast, native delay/accepted summon, real stun/expiry, native49101.
-- Keep the actual204771 spellclick and all104276 combat/finale rows unchanged.
DELETE FROM smart_scripts WHERE entryorguid=103398 AND @stromkar_awake;
UPDATE creature_template SET ScriptName='npc_stromkar_awakening' WHERE entry=103398 AND @stromkar_awake;
INSERT INTO spell_script_names(spell_id,ScriptName) SELECT 206360,'spell_stromkar_opening_recoil' WHERE @stromkar_awake;
-- A late/cancelled/abandoned jump must not stun a player in the combat stage.
INSERT INTO conditions(SourceTypeOrReferenceId,SourceEntry,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,Comment)
SELECT 17,206361,32,0,16,0,'Stromkar awakening: actual living Warrior in active initial stage only' WHERE @stromkar_awake
UNION ALL SELECT 17,206361,15,0,1,0,'Stromkar awakening: actual living Warrior in active initial stage only' WHERE @stromkar_awake
UNION ALL SELECT 17,206361,22,0,1539,0,'Stromkar awakening: actual living Warrior in active initial stage only' WHERE @stromkar_awake
UNION ALL SELECT 17,206361,9,0,41105,0,'Stromkar awakening: actual living Warrior in active initial stage only' WHERE @stromkar_awake
UNION ALL SELECT 17,206361,36,0,0,0,'Stromkar awakening: actual living Warrior in active initial stage only' WHERE @stromkar_awake
UNION ALL SELECT 17,206361,44,0,1037,7,'Stromkar awakening: actual living Warrior in active initial stage only' WHERE @stromkar_awake;
COMMIT;
