-- Accessories of the Cursed (38324): link the existing sixteen bone piles
-- to their existing quest loot. questItem1/questID do not select FillLoot.
-- Preserve personal/group settings, native item counts and respawn behavior.
UPDATE gameobject_template g
SET g.Data1=g.entry
WHERE g.entry=240586 AND g.type=3 AND g.Data1=0
  AND g.questItem1=122611 AND g.Data8=38324 AND g.Data0=2160
  AND EXISTS (SELECT 1 FROM gameobject_loot_template l
              WHERE l.Entry=g.entry AND l.Item=122611 AND l.Currency=0
                AND l.Reference=0 AND l.Chance=100 AND l.QuestRequired=1
                AND l.LootMode=1 AND l.GroupId=0 AND l.MinCount=1 AND l.MaxCount=1);
