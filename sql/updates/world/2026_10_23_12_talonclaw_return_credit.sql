UPDATE quest_objectives SET Amount=1
WHERE ID=280794 AND QuestID=40385 AND Type=0 AND StorageIndex=1 AND ObjectID=100112 AND Amount=0 AND Flags=2;

UPDATE smart_scripts AS summon
JOIN smart_scripts AS credit ON credit.entryorguid=summon.entryorguid AND credit.source_type=summon.source_type AND credit.id=4
SET summon.link=0
WHERE summon.entryorguid=99480 AND summon.source_type=0 AND summon.id=3 AND summon.link=4
AND summon.event_type=61 AND summon.action_type=50 AND summon.action_param1=248834
AND credit.event_type=6 AND credit.action_type=33 AND credit.action_param1=103798 AND credit.target_type=21;

DELETE credit FROM smart_scripts AS credit
JOIN smart_scripts AS summon ON summon.entryorguid=credit.entryorguid AND summon.source_type=credit.source_type AND summon.id=3
WHERE credit.entryorguid=99480 AND credit.source_type=0 AND credit.id=4
AND credit.event_type=6 AND credit.action_type=33 AND credit.action_param1=103798 AND credit.target_type=21
AND summon.link=0 AND summon.event_type=61 AND summon.action_type=50 AND summon.action_param1=248834;
