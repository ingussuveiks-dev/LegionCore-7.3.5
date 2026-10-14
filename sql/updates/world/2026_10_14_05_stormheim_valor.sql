-- Private, quest-gated archive activation and confirmed recording creation.
-- Keep the native console combat/portal schedule and unrelated entry reuse.
UPDATE `gameobject_template` SET `ScriptName`='go_stormheim_valor_console'
WHERE `entry` IN (243808,243801) AND `ScriptName` IN ('','go_stormheim_valor_console');
UPDATE `creature_template` SET `ScriptName`='npc_stormheim_valor_actor'
WHERE `entry` IN (96122,96121,96078) AND `ScriptName` IN ('','npc_stormheim_valor_actor');
-- Restore the original 25549 objective, keeping its ID and storage slot so
-- existing character progress survives. Only replace the known NPC surrogate.
UPDATE `quest_objectives` SET `Type`=2, `ObjectID`=243801
WHERE `ID`=279720 AND `QuestID`=39591 AND `StorageIndex`=3 AND `Amount`=1
  AND `Type`=0 AND `ObjectID`=96078;
-- The installed projector was fully buried (Z 233.305, terrain 286.569;
-- native model 1255595 reaches only 21.839 units above its size-3 pivot).
-- Use the installed native recording destination, not an invented location.
UPDATE `gameobject` AS g
JOIN `spell_target_position` AS p ON p.`id`=190557 AND p.`target_map`=1220
SET g.`position_x`=p.`target_position_x`, g.`position_y`=p.`target_position_y`,
    g.`position_z`=p.`target_position_z`, g.`orientation`=p.`target_orientation`,
    g.`rotation0`=0, g.`rotation1`=0,
    g.`rotation2`=SIN(p.`target_orientation`/2), g.`rotation3`=COS(p.`target_orientation`/2)
WHERE g.`guid`=114823 AND g.`id`=243818 AND g.`map`=1220 AND g.`PhaseId`=''
  AND ABS(g.`position_x`-3479.78)<0.02 AND ABS(g.`position_y`-2130.51)<0.02
  AND ABS(g.`position_z`-233.305)<0.02 AND ABS(g.`orientation`-5.4982)<0.02
  AND g.`rotation0`=0 AND g.`rotation1`=0 AND g.`rotation2`=0 AND g.`rotation3`=1;
