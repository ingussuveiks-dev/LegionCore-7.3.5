-- A Mutual Friend: the messenger must appear BEFORE quest 44099 is accepted.
-- SpellArea treats quest_end_status as an allowed-state mask. Legacy 74
-- admitted incomplete/complete/rewarded, but excluded NONE (1 << 0).
-- Keep the native Dalaran City area, rewarded 40824 prerequisite and Warlock
-- restriction. Do not replace custom masks or touch other class messengers.
UPDATE spell_area
SET quest_end_status = 1
WHERE spell = 224338 AND area = 7581
  AND quest_start = 40824 AND quest_start_status = 64
  AND quest_end = 44099 AND quest_end_status = 74
  AND classmask = 256 AND autocast = 1;
