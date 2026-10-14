-- Actual scenario tome credit owns the archive confrontation. A transient
-- SmartAI phase from Nelach's linked death/dialogue list is not durable progress.
UPDATE creature_template SET ScriptName='npc_new_blood_archive_guide' WHERE entry=99178 AND AIName='SmartAI';
DELETE FROM smart_scripts WHERE entryorguid=99178 AND source_type=0 AND id=5;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=99178 AND SourceGroup=6 AND SourceId=0;
DELETE FROM smart_scripts WHERE entryorguid=9917802 AND source_type=9;
