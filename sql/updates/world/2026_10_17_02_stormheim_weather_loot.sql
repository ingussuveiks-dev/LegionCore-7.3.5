-- To Weather the Storm (38405/38614): select the existing personal quest
-- loot for both faction templates. Personal markers 60122/60124 are not IDs
-- for GetLootId. Keep their locks, quest restrictions, scale and spawns.
UPDATE gameobject_template g
SET g.Data1=g.entry
WHERE g.entry IN(240677,240681,241460,241462) AND g.type=3 AND g.Data1=0
  AND g.Data0=1690
  AND ((g.entry IN(240677,241460) AND g.questItem1=122685)
    OR (g.entry IN(240681,241462) AND g.questItem1=122686))
  AND EXISTS (SELECT 1 FROM gameobject_loot_template l
              WHERE l.Entry=g.entry AND l.Item=g.questItem1 AND l.Currency=0
                AND l.Reference=0 AND l.Chance=100 AND l.QuestRequired=1
                AND l.LootMode=1 AND l.GroupId=0 AND l.MinCount=1 AND l.MaxCount=1);

-- These four empty self-exploration placeholders have no live StartScript
-- hook, but the script loader still restores the exploration requirement.
-- Item collection alone must complete 38614. Preserve custom script lists.
DELETE FROM quest_start_scripts
WHERE id=38614 AND delay=0 AND command=7 AND datalong=38614
  AND datalong2=0 AND dataint=0 AND x=0 AND y=0 AND z=0 AND o=0;
UPDATE quest_template SET StartScript=0
WHERE ID=38614 AND StartScript=38614
  AND NOT EXISTS(SELECT 1 FROM quest_start_scripts WHERE id=38614);
UPDATE quest_template_addon SET SpecialFlags=SpecialFlags & ~2
WHERE ID=38614 AND SpecialFlags=2
  AND NOT EXISTS(SELECT 1 FROM quest_start_scripts WHERE command=7 AND datalong=38614);
