-- Restore the native 7.3.5 required/sequenced witness and hidden optional wait.
UPDATE quest_objectives SET Flags=3 WHERE ID=280508 AND QuestID=39387;
UPDATE quest_objectives SET Flags=12 WHERE ID=280509 AND QuestID=39387;

-- Shared starter remains available; the summoned Lasan runs an owner-only pledge.
-- Remove immediate gossip credit and 40-yard bystander credit from the old AI.
DELETE FROM smart_scripts WHERE entryorguid=98773 AND source_type=0;
UPDATE creature_template SET AIName='',ScriptName='npc_skyhorn_ritual_guide' WHERE entry=98773;

-- The old object-ID condition matched the meeting objective, hiding Lasan
-- immediately after the first credit and preventing retries after interruption.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=23 AND
 ((SourceGroup=7503 AND SourceEntry=26) OR (SourceGroup=7731 AND SourceEntry=4));
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionValue1,Comment) VALUES
(23,7503,26,0,9,39387,'Skyhorn ritual - quest incomplete'),
(23,7503,26,1,28,39387,'Skyhorn ritual - quest complete until reward'),
(23,7731,4,0,9,39387,'Skyhorn ritual - quest incomplete'),
(23,7731,4,1,28,39387,'Skyhorn ritual - quest complete until reward');

-- The missing introductory pledge line is present in 7.3.5 BroadcastText.
DELETE FROM creature_text WHERE CreatureID=98773 AND GroupID=3;
INSERT INTO creature_text (CreatureID,GroupID,ID,Text,Type,Language,Probability,BroadcastTextID,comment) VALUES
(98773,3,0,'I have already ordered my eagle riders to make for Thunder Totem. You may find the air a bit thicker with feathers soon.',12,0,100,100851,'Lasan - Skyhorn pledge introduction');
