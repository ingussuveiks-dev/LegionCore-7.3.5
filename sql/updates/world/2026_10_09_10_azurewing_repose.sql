-- Existing native loot was unreachable: a zero chestLoot never calls FillLoot.
UPDATE gameobject_template SET Data1=entry WHERE entry IN(240033,240267) AND type=3 AND Data1=0;

-- 7.3.5 TDB 25549 objective flags; retain storage indices to preserve saved progress.
UPDATE quest_objectives SET Flags2=1 WHERE ID IN(277011,277144,277268,284014,284016) AND Flags2=0;
UPDATE quest_objectives SET Flags=28 WHERE ID=284778 AND QuestID=37862 AND ObjectID=107995 AND Flags=2;
UPDATE quest_objectives SET Amount=1 WHERE ID=280835 AND QuestID=37861 AND ObjectID=100377 AND Amount=0;

DELETE FROM spell_script_names WHERE (spell_id=179915 AND ScriptName='spell_azurewing_pool')
OR (spell_id IN(180713,180463) AND ScriptName='spell_azurewing_revive')
OR (spell_id=179825 AND ScriptName='spell_azurewing_pylon')
OR (spell_id=180515 AND ScriptName='spell_azurewing_whelp_pickup');
INSERT INTO spell_script_names (spell_id,ScriptName) VALUES
(179915,'spell_azurewing_pool'),(180713,'spell_azurewing_revive'),
(180463,'spell_azurewing_revive'),(179825,'spell_azurewing_pylon'),(180515,'spell_azurewing_whelp_pickup');
-- The actual button is 179915 (OverrideSpellData 573), not generic dummy 6967.
DELETE FROM smart_scripts WHERE entryorguid=89975 AND source_type=0 AND id=0 AND event_type=8 AND event_param1=6967 AND action_type=33 AND action_param1=90315;

-- Native pylons are the only legal targets of the destination-area dummy effect.
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionValue1,ConditionValue2,Comment)
SELECT 13,1,179825,p.g,31,3,p.entry,'Azurewing: native mana pylon target'
FROM (SELECT 0 g,90263 entry UNION ALL SELECT 1,100383 UNION ALL SELECT 2,100384 UNION ALL SELECT 3,100385) p
WHERE NOT EXISTS (SELECT 1 FROM conditions c WHERE c.SourceTypeOrReferenceId=13 AND c.SourceGroup=1 AND c.SourceEntry=179825 AND c.ElseGroup=p.g AND c.ConditionTypeOrReference=31 AND c.ConditionValue2=p.entry);

UPDATE creature_template SET AIName='',ScriptName='npc_azurewing_runas_duel' WHERE entry=90372 AND ScriptName IN('','npc_azurewing_runas_duel');
UPDATE creature_template SET AIName='',ScriptName='npc_azurewing_runas_follower' WHERE entry=90476 AND ScriptName IN('','npc_azurewing_runas_follower');
UPDATE creature_template SET AIName='',ScriptName='npc_azurewing_runas_guide' WHERE entry=90406 AND ScriptName IN('','npc_azurewing_runas_guide');
UPDATE creature_template SET AIName='',ScriptName='npc_azurewing_runas_start' WHERE entry=90383 AND ScriptName IN('','npc_azurewing_runas_start');
-- This area summon ran before Runas was defeated and credited arrival by timer.
DELETE FROM spell_area WHERE spell=180066 AND area=7338 AND quest_start=37957;
-- Stellagosa must be visible before accepting Runas the Shamed, including on
-- an abandoned attempt. The old negated QUEST_NONE condition required it first.
UPDATE conditions SET NegativeCondition=0 WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7334 AND SourceEntry=15
AND ConditionTypeOrReference=14 AND ConditionValue1=37957 AND NegativeCondition=1;

-- Runas Knows the Way follows both whelplands tasks, in addition to PrevQuestID 37957.
INSERT INTO conditions (SourceTypeOrReferenceId,SourceEntry,ConditionTypeOrReference,ConditionValue1,Comment)
SELECT 19,37857,8,q.id,'Azurewing: finish whelplands tasks before following Runas'
FROM (SELECT 37856 id UNION ALL SELECT 37859) q
WHERE NOT EXISTS (SELECT 1 FROM conditions c WHERE c.SourceTypeOrReferenceId=19 AND c.SourceEntry=37857 AND c.ConditionTypeOrReference=8 AND c.ConditionValue1=q.id);

UPDATE creature_template SET AIName='',ScriptName='npc_azurewing_stellagosa_return'
WHERE entry=107995 AND ScriptName IN('npc_azsuna_stellagosa_q37862','npc_azurewing_stellagosa_return');
-- A complete, un-rewarded delivery quest can still use its optional ride.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7334 AND SourceEntry=139
AND ConditionTypeOrReference=28 AND ConditionValue1=37862 AND NegativeCondition=1;

-- Orbyth and Ael'Yith are distinct sequential enemies, not aliases for each other.
UPDATE creature_template_wdb SET KillCredit1=0 WHERE Entry=91155 AND KillCredit1=108721;
UPDATE creature_template_wdb SET KillCredit1=0 WHERE Entry=108721 AND KillCredit1=91155;
DELETE FROM npc_spellclick_spells WHERE npc_entry=91155 AND spell_id=46598;
DELETE FROM creature WHERE guid=338484 AND id=91155 AND map=1220;
UPDATE creature_template SET AIName='',ScriptName='npc_azurewing_final_enemy'
WHERE entry IN(91155,108721) AND ScriptName IN('','npc_azurewing_final_enemy');
