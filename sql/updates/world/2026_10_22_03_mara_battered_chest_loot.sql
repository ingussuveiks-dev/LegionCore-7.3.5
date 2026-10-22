-- Mara Swiftmane's summoned reward chest has loot rows but no template link.
-- Retain the existing rewards, probabilities, personal-loot and encounter rules.
UPDATE gameobject_template SET Data1=245479 WHERE entry=245479 AND type=3;
