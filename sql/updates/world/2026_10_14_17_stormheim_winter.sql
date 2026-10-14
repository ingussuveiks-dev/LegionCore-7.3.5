-- Both ordinary grapple point types still used Keg Breaking. Preserve custom
-- click rows and the native gun range, hook, charge, jump and tether graph.
UPDATE npc_spellclick_spells a LEFT JOIN npc_spellclick_spells b ON b.npc_entry=a.npc_entry AND b.spell_id=182478
SET a.spell_id=182478 WHERE a.npc_entry IN(92017,92072) AND a.spell_id=121975 AND a.cast_flags=1 AND a.user_type=0 AND a.add_npc_flag=1 AND b.npc_entry IS NULL;
UPDATE eventobject_template SET ScriptName='eventobject_stormheim_winter'
WHERE entry IN(611,612) AND ScriptName IN('SmartEventObject','eventobject_stormheim_winter')
AND radius=IF(entry=611,5,10) AND SpellID=0 AND WorldSafeLocID=0 AND Flags=0;
-- This hook is empty, not a self-completion action. Preserve custom lists.
UPDATE quest_template SET StartScript=0 WHERE ID=38618 AND StartScript=38618
AND NOT EXISTS(SELECT 1 FROM quest_start_scripts WHERE id=38618);
