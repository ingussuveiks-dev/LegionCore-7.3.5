-- Nar'thalas entry: retain native objectives and item/spell IDs (7.3.5).
UPDATE gameobject_template SET Data1=entry
WHERE entry IN (239744,239745) AND type=3 AND Data1=0;

-- The TDB 735 objectives flag these four items as quest-bound inventory.
-- Retain StorageIndex: existing player saves use this core's indices.
UPDATE quest_objectives SET Flags2=1
WHERE QuestID=37736 AND ID IN (276813,276814,276815,276816) AND Type=1;

UPDATE creature_template SET AIName='',ScriptName='npc_narthalas_farondis_walk'
WHERE entry=88889 AND ScriptName IN ('','npc_narthalas_farondis_walk');
DELETE FROM smart_scripts WHERE (entryorguid=88889 AND source_type=0)
    OR (entryorguid=8888900 AND source_type=9);
-- One player's escort must not hide the shared questgiver from everyone.
UPDATE smart_scripts SET link=0 WHERE entryorguid=88867 AND source_type=0 AND id=1
    AND event_type=62 AND event_param1=17377 AND event_param2=1 AND action_type=85 AND action_param1=177816;
DELETE FROM smart_scripts WHERE entryorguid=88867 AND source_type=0 AND id IN (2,3) AND action_type=47;

DELETE FROM spell_script_names WHERE (spell_id=177816 AND ScriptName='spell_narthalas_start_walk')
    OR (spell_id=179185 AND ScriptName='spell_narthalas_borrow_robes')
    OR (spell_id=179203 AND ScriptName='spell_narthalas_dressing_complete');
INSERT INTO spell_script_names (spell_id,ScriptName) VALUES
(177816,'spell_narthalas_start_walk'),
(179185,'spell_narthalas_borrow_robes'),
(179203,'spell_narthalas_dressing_complete');
UPDATE spell_loot_template SET QuestRequired=1 WHERE Entry=179185 AND Item=120948;
