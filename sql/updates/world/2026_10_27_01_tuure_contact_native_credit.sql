-- 209441 already owns personal105697 credit and native taxi5626.
-- ON_SPELLCLICK runs even after rejected conditions/casts: its action33 gave
-- premature/group credit and followed nonexistent LINK1. Keep the native cast.
START TRANSACTION;
SET @tuure_contact_safe := EXISTS(SELECT 1 FROM creature_template
 WHERE entry=105697 AND AIName='SmartAI' AND ScriptName='')
 AND (SELECT COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=105697)=1
 AND EXISTS(SELECT 1 FROM npc_spellclick_spells WHERE npc_entry=105697
 AND spell_id=209441 AND cast_flags=1 AND user_type=0 AND add_npc_flag=1)
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=105697 AND source_type=0
 AND NOT(id=0 AND link=1 AND Difficulties='' AND event_type=73 AND event_phase_mask=0
 AND event_chance=100 AND event_flags=0 AND event_param1=0 AND event_param2=0
 AND event_param3=0 AND event_param4=0 AND event_param5=0 AND action_type=33
 AND action_param1=105697 AND action_param2=0 AND action_param3=0 AND action_param4=0
 AND action_param5=0 AND action_param6=0 AND target_type=7 AND target_param1=0
 AND target_param2=0 AND target_param3=0 AND target_param4=0
 AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0));
SET @tuure_contact_guard := IF(@tuure_contact_safe,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Tuure contact custom/missing native metadata; review before migration''');
PREPARE tuure_contact_check FROM @tuure_contact_guard;
EXECUTE tuure_contact_check;
DEALLOCATE PREPARE tuure_contact_check;
DELETE FROM smart_scripts WHERE @tuure_contact_safe AND entryorguid=105697 AND source_type=0;
COMMIT;
