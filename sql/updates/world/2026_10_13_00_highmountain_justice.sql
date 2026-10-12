-- Justice Rains from Above: start only after confirmed boarding, then follow
-- the existing bombing circuit and land safely beside the live Lasan spawn.
UPDATE `creature_template` SET `AIName`='',`ScriptName`='npc_highmountain_justice'
WHERE `entry`=100711 AND `AIName` IN ('','SmartAI')
  AND `ScriptName` IN ('','npc_highmountain_justice');
-- Remove only the obsolete summon/instant-path/despawn sequence.
DELETE FROM `smart_scripts` WHERE `entryorguid`=100711 AND `source_type`=0
  AND ((`id`=0 AND `event_type`=54 AND `action_type`=85 AND `action_param1`=46598)
    OR (`id`=1 AND `event_type`=61 AND `action_type`=53 AND `action_param2`=100711)
    OR (`id`=2 AND `event_type`=40 AND `event_param1`=72 AND `event_param2`=100711 AND `action_type`=41)
    OR (`id`=3 AND `event_type`=61 AND `action_type`=41 AND `action_param1`=300000));
DELETE FROM `smart_scripts` WHERE `entryorguid`=100712 AND `source_type`=0 AND `id`=0
  AND `event_type`=73 AND `action_type`=33 AND `action_param1`=100712;
INSERT IGNORE INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
 (198214,'spell_highmountain_justice'),(195024,'spell_highmountain_justice_heal');
UPDATE `creature_template` SET `ScriptName`='npc_highmountain_justice_wounded'
WHERE `entry` IN (97498,97506) AND `AIName`='' AND `ScriptName` IN ('','npc_highmountain_justice_wounded');
-- The outgoing quest must follow this chain, not the unrelated 39027 branch.
UPDATE `quest_template_addon` SET `PrevQuestID`=40594 WHERE `ID`=39780 AND `PrevQuestID`=39027;
