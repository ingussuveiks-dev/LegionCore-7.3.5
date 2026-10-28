START TRANSACTION;
SET @stromkar_ritual_receiver :=
 EXISTS(SELECT 1 FROM creature_template WHERE entry=101875 AND AIName='SmartAI' AND ScriptName='')
 AND NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=201031)
 AND EXISTS(SELECT 1 FROM creature WHERE guid=370922 AND id=103144 AND map=1539 AND PhaseId='5739')
 AND (SELECT COUNT(*) FROM creature WHERE guid IN(370936,370937,370943) AND id=101875 AND map=1539 AND PhaseId='5739')=3
 AND EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=101875 AND source_type=0 AND id=6 AND link=0 AND Difficulties=''
  AND event_type=60 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1 AND event_param1=1 AND event_param2=70
  AND event_param3=0 AND event_param4=0 AND event_param5=0 AND action_type=11 AND action_param1=201032 AND action_param2=0
  AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0 AND target_type=19 AND target_param1=103144
  AND target_param2=50 AND target_param3=0 AND target_param4=0 AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0);
INSERT INTO spell_script_names(spell_id,ScriptName) SELECT 201031,'spell_stromkar_ritual_beam' WHERE @stromkar_ritual_receiver;
-- OOC retry keeps missing/reloaded receivers recoverable without interrupting
-- an accepted infinite channel; native ordinary casts reject SPELL_IN_PROGRESS.
-- Do not reuse the health-threshold-only cast flag256 on an UPDATE event.
UPDATE smart_scripts SET event_type=1,event_flags=0,event_param3=1000,event_param4=1000 WHERE entryorguid=101875 AND source_type=0 AND id=6 AND @stromkar_ritual_receiver;
COMMIT;
