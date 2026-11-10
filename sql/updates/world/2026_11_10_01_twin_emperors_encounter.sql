-- Native26972 DungeonEncounter715:Map531,Bit6,Twin Emperors
-- Preserve this matching Legion database's real Vek'lor15276 kill credit
-- Its lone difficulty3 row is10Normal,while this map natively uses40Player
-- Keep outside the live updater until the native paired-Kill executable is promoted
SET @legion_twin_encounter_guard = IF(
  (SELECT COUNT(*) FROM instance_encounters WHERE entry=715)>1 OR
  EXISTS (SELECT 1 FROM instance_encounters WHERE entry=715 AND
    NOT (difficulty IN(-1,3) AND creditType=0 AND creditEntry=15276 AND lastEncounterDungeon=0)),
  'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Twin Emperors715 has unexpected encounter data: review before repair''',
  'DO 0');
PREPARE legion_twin_encounter_guard FROM @legion_twin_encounter_guard;
EXECUTE legion_twin_encounter_guard;
DEALLOCATE PREPARE legion_twin_encounter_guard;
START TRANSACTION;
UPDATE instance_encounters SET difficulty=-1
WHERE entry=715 AND difficulty=3 AND creditType=0 AND creditEntry=15276 AND lastEncounterDungeon=0;
INSERT INTO instance_encounters (entry,difficulty,creditType,creditEntry,lastEncounterDungeon,comment)
SELECT 715,-1,0,15276,0,'Twin Emperors: native Veklor death'
WHERE NOT EXISTS (SELECT 1 FROM instance_encounters WHERE entry=715);
COMMIT;
SET @legion_twin_encounter_guard = NULL;
