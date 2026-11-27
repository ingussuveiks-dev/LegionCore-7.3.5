-- The derived SmartAI owns the exact incantation option.
START TRANSACTION;

UPDATE `creature_template`
SET `ScriptName` = 'npc_faronaar_meryl_ritual'
WHERE `entry` = 108097 AND `AIName` = 'SmartAI' AND `ScriptName` = '' AND `gossip_menu_id` = 19816;

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 108097 AND `source_type` = 0
  AND ((`id` = 10 AND `link` = 15 AND `event_type` = 62 AND `event_param1` = 19816
        AND `event_param2` = 0 AND `action_type` = 34 AND `action_param1` = 108855)
    OR (`id` = 15 AND `link` = 0 AND `event_type` = 61 AND `action_type` = 72));

DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 22 AND `SourceGroup` = 11 AND `SourceEntry` = 108097 AND `SourceId` = 0
  AND `ElseGroup` = 0 AND `ConditionTypeOrReference` IN (9, 15, 36, 44);

COMMIT;
