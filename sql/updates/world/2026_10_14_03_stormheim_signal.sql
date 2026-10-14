-- Signal Boost: validate the actual propeller interaction, keep its encounter
-- private, acknowledge help's completed flight, and protect Vethir's surrender.
UPDATE gameobject_template SET ScriptName='go_stormheim_signal'
WHERE entry=240235 AND ScriptName IN ('','go_stormheim_signal');
UPDATE creature_template SET ScriptName='npc_stormheim_signal_controller'
WHERE entry=92475 AND ScriptName IN ('','npc_stormheim_signal_controller');
UPDATE creature_template SET ScriptName='npc_stormheim_signal_actor'
WHERE entry IN (90899,90907,92547,92556,92557) AND ScriptName IN ('','npc_stormheim_signal_actor');

-- The two propellers occupy identical XY with a ten-yard Z offset. Keep the
-- established ground-level spawn; remove only the exact elevated duplicate.
DELETE duplicate FROM gameobject duplicate JOIN gameobject original ON original.guid=127609
WHERE duplicate.guid=114500 AND duplicate.id=240235 AND original.id=240235
AND duplicate.map=1220 AND original.map=1220 AND duplicate.phaseMask=original.phaseMask
AND duplicate.PhaseId=original.PhaseId AND ABS(duplicate.position_x-original.position_x)<0.01
AND ABS(duplicate.position_y-original.position_y)<0.01
AND duplicate.spawnMask=original.spawnMask AND duplicate.state=original.state
AND duplicate.personal_size=original.personal_size AND ABS(duplicate.orientation-original.orientation)<0.01
AND ABS(duplicate.position_z-original.position_z-10)<0.01;

-- A Grim Trophy retains its real quest-required head loot, including the
-- valid LootMode=0. No scripted kill credit substitutes for owning the item.
