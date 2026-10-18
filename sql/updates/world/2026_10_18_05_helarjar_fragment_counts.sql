-- Reconstructed rewards, not a recovered Blizzard loot table.
-- Rockaway: original2016 samples12/14/15 disprove fixed15; use10..15.
-- Helya: original2016/2017 ~150 samples disprove30..60; use100..150,
-- consistent with later observed100..150 quantities. Exact retail bounds
-- remain unverified in docs/audits/helarjar-fragment-counts-2026-10-18.json.
-- Keep chance, quest eligibility, loot modes, groups and all other sources.
UPDATE spell_loot_template
SET MinCount=10,MaxCount=15,
    Comment='Rockaway completion; reconstructed10..15; exact retail bounds unverified'
WHERE Entry=229113 AND Item=142487 AND Currency=0;

UPDATE creature_loot_template
SET MinCount=100,MaxCount=150
WHERE Entry=114537 AND Item=142487 AND Currency=0;
