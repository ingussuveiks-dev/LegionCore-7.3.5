-- Native effect249 equips an existing item; it does not create the artifact.
-- Keep each SmartGameObjectAI's loot-completed visuals and progression chain.
UPDATE gameobject_template SET Data1=entry,ScriptName='go_artifact_acquisition'
WHERE type=3 AND entry IN(249812,248572,246699,249814,251049,246021,249821,246437)
AND ScriptName IN('','go_artifact_acquisition') AND AIName='SmartGameObjectAI' AND Data1 IN(0,entry);

-- Only the real completed-loot event should run pickup progression. A gossip
-- cast could advance these three objects even with a full bag and no weapon.
DELETE FROM smart_scripts WHERE source_type=1 AND event_type=64 AND action_type=85 AND
((entryorguid=249812 AND id=2 AND action_param1=212010) OR
 (entryorguid=248572 AND id=2 AND action_param1=208011) OR
 (entryorguid=246437 AND id=1 AND action_param1=205387));
