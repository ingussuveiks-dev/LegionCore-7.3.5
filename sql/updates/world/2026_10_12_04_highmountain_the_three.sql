-- Both Haglands assignments follow Rocs vs Eagles; The Witchqueen requires both.
UPDATE quest_template_addon SET PrevQuestID=38910,NextQuestID=39322,ExclusiveGroup=-39321
WHERE ID IN(39321,39429);
UPDATE quest_template_addon SET PrevQuestID=39321 WHERE ID=39322 AND PrevQuestID=39429;

-- SmartAI does not schedule creature_template_spell entries automatically.
-- Retain each existing out-of-combat aura; add the native combat ability.
-- Timings are server tuning, not recovered retail sniff timings.
DELETE FROM smart_scripts WHERE entryorguid IN(95194,95195) AND source_type=0 AND id=1;
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,event_chance,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,target_type,comment) VALUES
(95194,0,1,0,100,3000,3000,15000,15000,11,192111,1,'Ragi the Hexxer - in combat - native area Hex'),
(95195,0,1,0,100,3000,3000,12000,12000,11,194228,1,'Agara Deathsong - in combat - native Deathsong');

-- These native dummy effects encode their next spell in EffectBasePoints.
-- Option 6 casts it normally as triggered, preserving the child's own damage.
DELETE FROM spell_dummy_trigger WHERE spell_id IN(194241,215591) AND spell_trigger IN(215588,215592) AND `option`=6 AND aura=0;
INSERT INTO spell_dummy_trigger (spell_id,spell_trigger,`option`,effectmask,handlemask,comment) VALUES
(194241,215588,6,1,8,'Deathsong - native dummy basepoints -> native damage spell on hit target'),
(215591,215592,6,1,8,'Foul Blast - native dummy basepoints -> missile -> damage and knockback');
