-- The previously incomplete custom service creature has an existing spawn at
-- Krasus' Landing (-827.481, 4324.84, 744.952). Offer the same guarded recovery
-- there and at its existing capital spawns for players stranded by the old
-- Holgar gossip-index collision.
UPDATE `creature_template` SET `ScriptName` = 'npc_broken_shore_return'
WHERE `entry` = 230005 AND `ScriptName` IN ('', 'npc_broken_shore_return');

UPDATE `creature_template_wdb`
SET `Name1` = 'Broken Shore Guide', `Title` = 'Legion Introduction'
WHERE `Entry` = 230005 AND `Name1` IN ('', 'Service NPC', 'Broken Shore Guide');
