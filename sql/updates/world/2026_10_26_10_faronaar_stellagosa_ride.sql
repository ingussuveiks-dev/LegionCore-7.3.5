-- Keep outside the live updater until the matching ride AI/spell are deployed.
START TRANSACTION;
SET @faronaar_ride := EXISTS (SELECT 1 FROM creature_template WHERE entry=90982 AND VehicleId=4103 AND AIName='SmartAI' AND ScriptName IN ('','npc_faronaar_stellagosa_ride'))
AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=180768 AND ScriptName<>'spell_faronaar_stellagosa_ride')
AND EXISTS (SELECT 1 FROM vehicle_template_accessory WHERE EntryOrAura=90982 AND accessory_entry=90983 AND seat_id=0 AND ABS(offsetX-0.98969)<0.00001 AND offsetY=0 AND ABS(offsetZ-3.84163)<0.00001 AND minion=1 AND summontype=8)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=90982 AND source_type=0 AND id=1 AND event_type=27 AND event_flags=1 AND action_type=53 AND action_param2=90982 AND target_type=7)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=90982 AND source_type=0 AND id=5 AND event_type=27 AND event_flags=1 AND action_type=33 AND action_param1=90982 AND target_type=7);

UPDATE creature_template SET ScriptName='npc_faronaar_stellagosa_ride' WHERE entry=90982 AND @faronaar_ride;
INSERT INTO spell_script_names (spell_id,ScriptName)
SELECT 180768,'spell_faronaar_stellagosa_ride' WHERE @faronaar_ride
AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=180768 AND ScriptName='spell_faronaar_stellagosa_ride');

-- Native seat15333/slot0 holds Kor'vas. Seat15334/slot1 is the player's
-- usable seat; this second accessory copied the same NPC into that seat.
DELETE FROM vehicle_template_accessory
WHERE @faronaar_ride AND EntryOrAura=90982 AND accessory_entry=90983 AND seat_id=1
AND offsetX=0 AND offsetY=0 AND offsetZ=0 AND offsetO=0 AND minion=0
AND description='Drake' AND summontype=8 AND summontimer=300;
COMMIT;
