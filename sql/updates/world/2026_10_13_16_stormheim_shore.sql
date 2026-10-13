-- Existing medical-supply loot was unreachable from the native chest.
UPDATE gameobject_template SET Data1=241275 WHERE entry=241275 AND type=3 AND Data1=0;
-- Restore the native optional/hidden waste counter; only a full container is
-- required for completion. Preserve any additional locally configured bits.
UPDATE quest_objectives SET Flags=Flags | 12
 WHERE ID=277818 AND QuestID=38358 AND Type=0 AND ObjectID=91693 AND Amount=100;

-- The script casts native instant Pump after the native open-lock completes.
-- Keep ordinary goober animation/cooldown, but suppress its second cast.
UPDATE gameobject_template SET ScriptName='go_stormheim_volatile_barrel',Data10=0
 WHERE entry IN(240600,249829) AND type=10 AND Data10 IN(0,181981)
 AND ScriptName IN('','go_stormheim_volatile_barrel');
DELETE FROM spell_script_names WHERE spell_id=181981 AND ScriptName='spell_stormheim_shore_pump';
INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(181981,'spell_stormheim_shore_pump');
