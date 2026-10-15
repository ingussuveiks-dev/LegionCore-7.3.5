-- Regal Remains (38817): link the five existing bone chests to their loot.
-- questItem1/questID alone do not make Player::SendLoot call FillLoot.
-- Native Roll treats Chance>=100 as guaranteed; retain the existing100.185.
UPDATE gameobject_template g
SET g.Data1 = g.entry
WHERE g.entry IN(241870,241871,241872,241873,241874)
  AND g.type=3 AND g.Data1=0 AND g.questItem1=124512 AND g.Data8=38817
  AND EXISTS (SELECT 1 FROM gameobject_loot_template l
              WHERE l.Entry=g.entry AND l.Item=124512 AND l.Currency=0
                AND l.Reference=0 AND l.Chance>=100 AND l.QuestRequired=1
                AND l.LootMode=1 AND l.GroupId=0 AND l.MinCount=1 AND l.MaxCount=1);
