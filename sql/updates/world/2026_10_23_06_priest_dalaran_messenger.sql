-- Hooded Priestess's native acceptance TALK points to missing LINK1.
-- Restore the same acceptance cleanup used by the other class messengers.
SET @priest_messenger = EXISTS (SELECT 1 FROM creature_template WHERE entry=101344 AND AIName='SmartAI' AND ScriptName='')
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=101344 AND source_type=0 AND id=0
 AND event_type=19 AND event_param1=40705 AND action_type=1 AND link=1);
INSERT INTO smart_scripts
 (entryorguid,source_type,id,link,event_type,event_chance,event_param1,action_type,action_param1,target_type,comment)
SELECT 101344,0,1,0,61,100,0,41,5000,1,'Priestly Matters - acceptance LINK - despawn after dialogue'
WHERE @priest_messenger AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=101344 AND source_type=0 AND id=1);

-- Native3850 summons the questgiver beside its player. Keep it available while
-- the player walks, personally visible, with the existing messenger timeout.
-- Do not graft a partial chain onto custom event slots or an existing bootstrap.
SET @priest_follow = @priest_messenger
AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=101344 AND source_type=0 AND (event_type=54 OR id IN (2,3,4)));
INSERT INTO smart_scripts
 (entryorguid,source_type,id,link,event_type,event_chance,action_type,action_param1,target_type,comment)
SELECT 101344,0,n.id,n.link,n.event_type,100,n.action_type,n.delay,n.target_type,n.comment
FROM (
 SELECT 2 AS id,3 AS link,54 AS event_type,29 AS action_type,0 AS delay,7 AS target_type,'Priestly Matters - summoned - follow player' AS comment UNION ALL
 SELECT 3,4,61,207,0,7,'Priestly Matters - LINK - personal visibility' UNION ALL
 SELECT 4,0,61,41,30000,1,'Priestly Matters - LINK - messenger timeout'
) n WHERE @priest_follow;
