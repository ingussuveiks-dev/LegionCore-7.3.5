-- Type 50 reads Data1 as the gameobject loot table ID.
-- Both Broken Shore containers already have their loot under their entry IDs.
UPDATE `gameobject_template` g
SET g.`Data1` = g.`entry`
WHERE g.`type` = 50
  AND ((g.`entry` = 271554 AND g.`Data1` = 72558)
    OR (g.`entry` = 271227 AND g.`Data1` = 71869))
  AND EXISTS (SELECT 1 FROM `gameobject_loot_template` l WHERE l.`Entry` = g.`entry`)
  AND NOT EXISTS (SELECT 1 FROM `gameobject_loot_template` l WHERE l.`Entry` = g.`Data1`);
