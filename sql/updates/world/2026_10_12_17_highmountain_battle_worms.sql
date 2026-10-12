-- Battle of Snowblind Mesa is a journey/turn-in quest in TDB735, without
-- numeric objectives. Remove only the known local zero-amount placeholder.
DELETE FROM quest_objectives WHERE ID=280011 AND QuestID=38915 AND Type=0
 AND StorageIndex=0 AND ObjectID=93846 AND Amount=0 AND Flags=0 AND Flags2=0
 AND Description='Follow Myla Highmountain on the Highlands of Blinding Snows';

-- Hooked Tongue effect 0 encodes native child 197253 in EffectBasePoints.
-- The child moves its CASTER to the target's front: the struck unit must cast
-- it back at Arxas, not make the worm jump toward the player.
-- Option 11 honors the selected caster/target and retains the child's native jump parameters (no damage override).
DELETE FROM spell_dummy_trigger WHERE spell_id=197252 AND spell_trigger=197253
 AND `option`=11 AND effectmask=1 AND handlemask=8 AND aura=0;
INSERT INTO spell_dummy_trigger (spell_id,spell_trigger,`option`,effectmask,handlemask,caster,target,comment) VALUES
(197252,197253,11,1,8,5,3,'Hooked Tongue - hit target jumps toward original caster');
