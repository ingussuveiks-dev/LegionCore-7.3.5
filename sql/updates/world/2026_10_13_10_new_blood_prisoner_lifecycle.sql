-- Preserve SmartAI's later archive scenes; the C++ extension only owns the
-- persistent prisoners of scenario 962, and resumes their list after landing.
UPDATE creature_template SET ScriptName='npc_new_blood_prisoner'
WHERE entry IN (98775,98783,98793);

-- Alive guards must be measured from the prisoner, not the approaching player.
UPDATE conditions SET ConditionTarget=1
WHERE SourceTypeOrReferenceId=18 AND SourceGroup IN (98775,98783,98793)
AND SourceEntry=195039 AND ConditionTypeOrReference=29;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=18
AND SourceGroup IN (98775,98783,98793) AND SourceEntry=195039 AND ConditionTypeOrReference=9;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment) VALUES
(18,98775,195039,9,0,40729,'New Blood - active rescue quest'),
(18,98783,195039,9,0,40729,'New Blood - active rescue quest'),
(18,98793,195039,9,0,40729,'New Blood - active rescue quest');

-- Hold their native elevated spawn positions without adding HoverHeight twice.
-- This is spawn-specific: later archive summons must continue to walk normally.
INSERT INTO creature_movement_override (SpawnId,Ground,Flight)
SELECT guid,1,1 FROM creature WHERE map=1522 AND id IN (98775,98783,98793)
ON DUPLICATE KEY UPDATE Ground=VALUES(Ground),Flight=VALUES(Flight);

-- 199994 is Lightning_Fel_Cast_Hand, not the portal's green fire.
-- The portal model 607277 has its own particle-bearing CustomSpell02 loop.
-- Native AnimKit 1244 plays that loop, on this portal only, at the warning.
UPDATE smart_scripts SET action_type=128,action_param1=1244,action_param2=1,
comment='New Blood - native portal fire loop before Jagganoth'
WHERE entryorguid=98797 AND source_type=0 AND id=13 AND event_type=61;
