-- Requires the matching storage-gated paired SpellScript/AuraScript executable.
-- Keep outside live updater input until that Release executable is promoted.
START TRANSACTION;
SET @ashbringer_storage_safe =
 (SELECT COUNT(*)=1 FROM gameobject_template WHERE entry=247318 AND type=3 AND Data1=0 AND BINARY ScriptName=BINARY 'go_bs_the_ashbringer')
 AND (SELECT COUNT(*)=1 FROM gameobject WHERE guid=128117 AND id=247318 AND map=1500)
 AND (SELECT COUNT(*)=1 FROM spell_loot_template WHERE Entry=199827 AND Item=120978 AND Currency=0 AND Reference=0 AND Chance=100 AND MinCount=1 AND MaxCount=1)
 AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=180850 AND BINARY ScriptName<>BINARY 'spell_bs_ashbringer_storage')
 AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE BINARY ScriptName=BINARY 'spell_bs_ashbringer_storage' AND spell_id<>180850)
 AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=199827);
SET @ashbringer_storage_guard=IF(COALESCE(@ashbringer_storage_safe,0),'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Ashbringer storage native source/binding differs; review before updating''');
PREPARE ashbringer_storage_guard FROM @ashbringer_storage_guard;
EXECUTE ashbringer_storage_guard;
DEALLOCATE PREPARE ashbringer_storage_guard;
INSERT INTO spell_script_names (spell_id,ScriptName)
SELECT 180850,'spell_bs_ashbringer_storage'
WHERE @ashbringer_storage_safe AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=180850 AND BINARY ScriptName=BINARY 'spell_bs_ashbringer_storage');
COMMIT;
