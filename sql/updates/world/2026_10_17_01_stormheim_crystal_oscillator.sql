-- See Ya Later, Oscillator (38558): the existing personal chest must select
-- its existing quest loot. Data30=60251 is a personal-loot marker, not a loot ID.
-- Preserve lock, flags, scale, personal handling, quest items and spawn data.
UPDATE gameobject_template g
SET g.Data1=g.entry
WHERE g.entry=240286 AND g.type=3 AND g.Data1=0
  AND g.questItem1=123887 AND g.Data0=1690
  AND EXISTS (SELECT 1 FROM gameobject_loot_template l
              WHERE l.Entry=g.entry AND l.Item=123887 AND l.Currency=0
                AND l.Reference=0 AND l.Chance=100 AND l.QuestRequired=1
                AND l.LootMode=1 AND l.GroupId=0 AND l.MinCount=1 AND l.MaxCount=1);
