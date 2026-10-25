START TRANSACTION;

SET @truthguard_travel_click := (SELECT COUNT(*) FROM npc_spellclick_spells WHERE npc_entry IN(105883,106203))=2
AND EXISTS (SELECT 1 FROM npc_spellclick_spells WHERE npc_entry=105883 AND spell_id=209884 AND cast_flags=1 AND user_type=0 AND add_npc_flag=1)
AND EXISTS (SELECT 1 FROM npc_spellclick_spells WHERE npc_entry=106203 AND spell_id=210358 AND cast_flags=1 AND user_type=0 AND add_npc_flag=1)
AND (SELECT COUNT(*) FROM creature_template WHERE entry IN(105883,106203) AND npcflag=16777216 AND VehicleId=0 AND AIName='' AND ScriptName='npc_sr_argent_hippogryph')=2
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup IN(105883,106203))
AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE source_type=0 AND entryorguid IN(105883,106203,-366029,-382186));

DELETE FROM npc_spellclick_spells WHERE npc_entry IN(105883,106203) AND @truthguard_travel_click;

COMMIT;
