-- The Skies of Highmountain: keep the native 194359 -> 98778 / 4428 / 16028 chain.
-- The route is reconstructed and geometry-tested; see the Aviash audit.
UPDATE creature_template SET AIName='', ScriptName='npc_highmountain_aviash' WHERE entry=98778;
DELETE FROM spell_script_names WHERE spell_id=194359 AND ScriptName='spell_highmountain_aviash';
INSERT INTO spell_script_names (spell_id, ScriptName) VALUES (194359,'spell_highmountain_aviash');
DELETE FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=98777 AND SourceEntry=194359;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1,Comment) VALUES
(18,98777,194359,9,39387,'Aviash ride requires The Skies of Highmountain in progress');
