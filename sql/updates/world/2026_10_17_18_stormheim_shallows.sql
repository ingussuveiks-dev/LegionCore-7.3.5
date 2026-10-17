-- Scavenging the Shallows (40046): the existing Shipwreck Debris loot table
-- contains Corroded Maritime Plaque (129174), but a zero chest selector skips
-- FillLoot. Spell 227818 only awards unrelated kill credit 114487.
-- Preserve the original drop chances, interaction and personal-loot settings,
-- and do not replace a deliberately configured nonzero custom selector.
UPDATE gameobject_template
SET Data1=252239
WHERE entry=252239 AND type=3 AND Data1=0;
