-- Preserve Oakin's other quests and the cave summon/path SmartAI events.
UPDATE creature_template SET AIName='',ScriptName='npc_audience_oakin' WHERE entry=95256;
DELETE FROM smart_scripts WHERE entryorguid=95256 AND source_type=0 AND id IN(0,1);
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=2 AND SourceEntry=95256 AND SourceId=0;
-- Removed shared proximity timeline credited every nearby player immediately.
DELETE FROM smart_scripts WHERE entryorguid=9525600 AND source_type=9;
