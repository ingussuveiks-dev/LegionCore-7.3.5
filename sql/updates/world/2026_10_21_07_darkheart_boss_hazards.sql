-- Close the other ground/aura paths identified in the complete dungeon audit.
DELETE FROM spell_script_names WHERE
 (spell_id=212464 AND ScriptName='spell_darkheart_mythic_nightfall') OR
 (spell_id=200289 AND ScriptName='spell_darkheart_growing_paranoia');
INSERT INTO spell_script_names (spell_id,ScriptName) VALUES
 (212464,'spell_darkheart_mythic_nightfall'),
 (200289,'spell_darkheart_growing_paranoia');

-- Heroic 8082 has the same growing-paranoia mechanism as Normal 5631.
DELETE FROM areatrigger_actions WHERE entry=8082 AND customEntry=10343 AND id=0;
INSERT INTO areatrigger_actions (entry,customEntry,id,moment,actionType,targetFlags,spellId,comment)
VALUES (8082,10343,0,1,0,49160,200329,'Growing Paranoia - another player triggers target fear/damage');

-- Loader keys templates by entry, not (entry,spellId,customEntry). Multiple
-- conflicting rows made the selected visual/radius/wind depend on read order.
-- Keep one complete Down Draft definition per native difficulty-specific entry.
DELETE FROM areatrigger_template WHERE entry IN(5529,6084,8722);
INSERT INTO areatrigger_template
 (entry,spellId,customEntry,VisualID,Radius,RadiusTarget,windSpeed,windType,comment) VALUES
 (5529,199345,10771,51790,50,50,-7,1,'Dresaron - Down Draft Heroic'),
 (6084,199345,10771,51790,50,50,-7,1,'Dresaron - Down Draft Normal'),
 (8722,199345,10771,51790,50,50,-7,1,'Dresaron - Down Draft Mythic');
DELETE FROM areatrigger_actions WHERE entry IN(5529,6084,8722) AND customEntry=10771 AND id IN(2,3);
INSERT INTO areatrigger_actions (entry,customEntry,id,moment,actionType,targetFlags,spellId,comment) VALUES
 (5529,10771,2,1,0,10,220855,'Down Draft - native player self damage'),
 (5529,10771,3,42,13,10,220855,'Down Draft - remove damage on leaving/end'),
 (6084,10771,2,1,0,10,220855,'Down Draft - native player self damage'),
 (6084,10771,3,42,13,10,220855,'Down Draft - remove damage on leaving/end'),
 (8722,10771,2,1,0,10,220855,'Down Draft - native player self damage'),
 (8722,10771,3,42,13,10,220855,'Down Draft - remove damage on leaving/end');
