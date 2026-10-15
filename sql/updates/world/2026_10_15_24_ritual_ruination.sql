-- Ritual Ruination43254: keep the native outdoor quest phases and six objectives.
-- The old click teleported before the native five-minute Fel Bat controller ended.
DELETE FROM smart_scripts WHERE entryorguid=110479 AND source_type=0 AND id IN(0,1);
DELETE FROM spell_script_names WHERE ScriptName IN('spell_ritual_fel_bat','spell_ritual_artifact','spell_ritual_altar','spell_ritual_escape');
INSERT INTO spell_script_names(spell_id,ScriptName) VALUES
(219917,'spell_ritual_fel_bat'),(219839,'spell_ritual_artifact'),
(219920,'spell_ritual_altar'),(219923,'spell_ritual_altar'),(220374,'spell_ritual_escape');

-- The chest already had artifact loot, but Data1=0 bypassed native FillLoot.
UPDATE gameobject_template SET Data1=252054,AIName='',ScriptName='go_ritual_scepter' WHERE entry=252054;
DELETE FROM smart_scripts WHERE entryorguid=252054 AND source_type=1 AND id IN(0,1);
UPDATE creature_template SET AIName='',ScriptName='npc_ritual_guldan_image' WHERE entry=110548;

-- Retire the introduction after the duel; retire the pickup phase after sabotage.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7543 AND
 ((SourceEntry=9 AND ConditionTypeOrReference=41 AND ConditionValue1=43254 AND ConditionValue2=110556) OR
  (SourceEntry=10 AND ConditionTypeOrReference=41 AND ConditionValue1=43254 AND ConditionValue2=110606));
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment) VALUES
(23,7543,9,0,0,41,0,43254,110556,1,1,'Ritual Ruination - introduction before Allaris defeat'),
(23,7543,10,0,0,41,0,43254,110606,1,1,'Ritual Ruination - altar before sabotage');

-- Re-spawning Gul'dan after the duel re-ran the introduction. The player
-- recovery hook now supplies the personal post-duel image and return portal.
UPDATE smart_scripts SET link=0 WHERE entryorguid=110556 AND source_type=0 AND id=7 AND link=8;
DELETE FROM smart_scripts WHERE entryorguid=110556 AND source_type=0 AND id=8 AND action_type IN(0,70);

DELETE FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=110533 AND SourceEntry=0 AND ConditionTypeOrReference=9 AND ConditionValue1=43254;
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment) VALUES
(15,110533,0,0,0,9,0,43254,0,0,0,'Ritual Ruination - only an active participant can start the introduction');
