-- High Crawliac: use the native parent cyclone, not its contact-damage child.
-- Explicit targets also avoid AggressorAI treating TARGET_UNIT_TARGET_ANY as self.
-- Cadences are server tuning, not recovered retail sniff timings.
UPDATE creature_template SET AIName='SmartAI' WHERE entry=95153 AND ScriptName='';
DELETE FROM smart_scripts WHERE entryorguid=95153 AND source_type=0 AND id IN(0,1,2);
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,event_chance,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,target_type,comment) VALUES
(95153,0,0,0,100,4000,4000,15000,15000,11,192173,1,'High Crawliac - Vicious Storm around caster'),
(95153,0,1,0,100,8000,8000,18000,18000,11,192177,1,'High Crawliac - Shredding Cyclone native parent'),
(95153,0,2,0,100,12000,12000,12000,12000,11,192287,2,'High Crawliac - A Feast of Eyes on victim');

-- Each dummy effect encodes its own child ID in native EffectBasePoints.
-- Option 6 preserves child values; one effect bit per child prevents 25 casts.
DELETE FROM spell_dummy_trigger WHERE spell_id=192177 AND spell_trigger BETWEEN 192179 AND 192183 AND `option`=6 AND aura=0;
INSERT INTO spell_dummy_trigger (spell_id,spell_trigger,`option`,effectmask,handlemask,comment) VALUES
(192177,192179,6,1,8,'Shredding Cyclone - native dummy effect 0'),
(192177,192180,6,2,8,'Shredding Cyclone - native dummy effect 1'),
(192177,192181,6,4,8,'Shredding Cyclone - native dummy effect 2'),
(192177,192182,6,8,8,'Shredding Cyclone - native dummy effect 3'),
(192177,192183,6,16,8,'Shredding Cyclone - native dummy effect 4');

-- Existing native AT templates/movement/visuals are retained. The script gate
-- isolates shared template 2736 from Brilliant Edge and any other caster.
DELETE FROM areatrigger_actions WHERE entry IN(2736,4889) AND customEntry=0 AND id=0;
INSERT INTO areatrigger_actions (entry,id,moment,actionType,targetFlags,spellId,comment) VALUES
(2736,0,1,0,4194306,192184,'Shredding Cyclone - contact damage; valid attack target + script gate'),
(4889,0,1,0,4194306,192184,'Shredding Cyclone - contact damage; valid attack target + script gate');
DELETE FROM areatrigger_scripts WHERE entry IN(7345,9605);
INSERT INTO areatrigger_scripts (entry,ScriptName) VALUES
(7345,'at_highmountain_shredding_cyclone'),
(9605,'at_highmountain_shredding_cyclone');
