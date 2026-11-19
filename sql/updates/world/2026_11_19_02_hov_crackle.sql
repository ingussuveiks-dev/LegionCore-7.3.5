-- Legion26972:199805 has dummy BasePoints0;the real missile199807 creates199808/AT5578 and periodic199818.
DROP TEMPORARY TABLE IF EXISTS combat_crackle_guard;
CREATE TEMPORARY TABLE combat_crackle_guard (valid TINYINT NOT NULL CHECK (valid=1));
INSERT INTO combat_crackle_guard SELECT IF((SELECT COUNT(*) FROM spell_dummy_trigger WHERE spell_id=199805)=1 AND (SELECT COUNT(*) FROM spell_dummy_trigger WHERE spell_id=199805 AND spell_trigger IN(0,199807) AND `option`=4 AND target=0 AND caster=0 AND targetaura=0 AND bp0=0 AND bp1=0 AND bp2=0 AND effectmask=1 AND handlemask=8 AND aura=0 AND `group`=0)=1,1,0);
UPDATE spell_dummy_trigger SET spell_trigger=199807 WHERE spell_id=199805 AND spell_trigger=0 AND `option`=4 AND target=0 AND caster=0 AND targetaura=0 AND bp0=0 AND bp1=0 AND bp2=0 AND effectmask=1 AND handlemask=8 AND aura=0 AND `group`=0;
DROP TEMPORARY TABLE combat_crackle_guard;
