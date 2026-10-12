-- 7.3.5 Soul Link heals from Soul Leech absorption, not split incoming damage.
-- The core now handles that healing for damage procs and Demon Skin recharge.
DELETE FROM `spell_script_names`
WHERE `spell_id` = 108446 AND `ScriptName` = 'spell_warl_soul_link';
