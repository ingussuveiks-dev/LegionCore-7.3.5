-- Requires the quest-owned summon cleanup introduced with c8c00961.
-- The ritual already summons its own council and portal. The unphased DB
-- copies stay visible after accepting 40729 and can receive the click instead
-- of the personal portal. Keep the waiting council in the existing pre-quest
-- phase (8392/200), which is removed on accept and restored on abandon.
UPDATE creature SET PhaseId='5436'
WHERE (map=1220 AND guid IN (146813192,146813194,146813198,146813200)
 OR map=1502 AND guid IN (146890500,146890497,146890506,146890507,146890499))
 AND id IN (98797,101454,101455,101456,101457) AND PhaseId='';

-- Map 1220 already has the correctly phased original questgiver, guid 365955.
DELETE FROM creature WHERE guid=146813196 AND id=101456 AND map=1220 AND PhaseId='';
DELETE FROM creature WHERE id=101433 AND
 ((guid=146813190 AND map=1220) OR (guid=146890511 AND map=1502));

-- A personal attempt must wait for the player. Previously the clickable
-- creature and visual disappeared after 140 seconds, and Ritssyn's timer
-- reset independently of the portal click. Abandon/reward/logout now owns
-- cleanup, so neither a late click nor a slow ritual needs that watchdog.
UPDATE smart_scripts SET action_param2=8,action_param3=0,
 comment='New Blood - summon quest-owned portal until attempt ends'
WHERE entryorguid=98797 AND source_type=0 AND id=2 AND action_type=12
 AND action_param1=101433 AND action_param2=1 AND action_param3=140000;
UPDATE smart_scripts SET action_param2=0,
 comment='New Blood - portal visual follows quest attempt'
WHERE entryorguid=98797 AND source_type=0 AND id=3 AND action_type=50
 AND action_param1=245958 AND action_param2=140;
DELETE FROM smart_scripts WHERE entryorguid=10145600 AND source_type=9 AND
 ((id=3 AND action_type=47 AND action_param1=0 AND event_param1=121000
   AND comment='New Blood - inline recovery - hide Ritssyn')
 OR (id BETWEEN 4 AND 6 AND comment='New Blood - inline Ritssyn recovery tail'));

-- A stale client click must not start the ritual without its active quest.
INSERT IGNORE INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,
  ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment)
VALUES
 (18,101433,199966,0,0,9,0,40729,'New Blood portal - active quest'),
 (22,1,101433,0,0,9,0,40729,'New Blood portal event - active quest');
