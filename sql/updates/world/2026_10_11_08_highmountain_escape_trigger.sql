-- Handle the missing 190982 periodic child of 190967 through the quest's authorized transport.
DELETE FROM spell_script_names WHERE spell_id=190967 AND ScriptName='aura_rock_escape_transport';
INSERT INTO spell_script_names (spell_id,ScriptName) VALUES (190967,'aura_rock_escape_transport');
