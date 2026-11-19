-- Existing Legion loot rows were unreachable through the five placed reward chests.
-- Preserve native encounter, personal-loot, difficulty and completion admission.
DROP TEMPORARY TABLE IF EXISTS challenge_chest_link_guard;
CREATE TEMPORARY TABLE challenge_chest_link_guard (valid TINYINT NOT NULL CHECK (valid=1));
INSERT INTO challenge_chest_link_guard SELECT IF((SELECT COUNT(*) FROM gameobject_template WHERE (entry=252686 AND Data25=1870 AND Data30=68475 OR entry=269843 AND Data25=2039 AND Data30=73076 OR entry=269852 AND Data25=1961 AND Data30=73091 OR entry=269871 AND Data25=2017 AND Data30=73082 OR entry=272689 AND Data25=2068 AND Data30=74517) AND type=3 AND Data1 IN(0,entry) AND EXISTS(SELECT 1 FROM gameobject_loot_template l WHERE l.Entry=gameobject_template.entry))=5,1,0);
UPDATE gameobject_template SET Data1=entry WHERE entry IN(252686,269843,269852,269871,272689) AND Data1=0;
DROP TEMPORARY TABLE challenge_chest_link_guard;
