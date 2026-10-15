-- The Dreaming Fungus (38810): Soulthorn already has its quest loot, but
-- Player::SendLoot never fills it while the chest's loot selector is zero.
-- Preserve the native lock, quest gate, spawn/respawn and shared loot settings.
-- Nonzero custom loot selectors are deliberately left alone.
UPDATE gameobject_template
SET Data1 = 241866
WHERE entry = 241866 AND type = 3 AND Data1 = 0
  AND questItem1 = 124504 AND Data8 = 38810
  AND EXISTS (SELECT 1 FROM gameobject_loot_template
              WHERE Entry = 241866 AND Item = 124504 AND Currency = 0
                AND Reference = 0 AND Chance = 100 AND QuestRequired = 1
                AND LootMode = 1 AND GroupId = 0 AND MinCount = 1 AND MaxCount = 1);
