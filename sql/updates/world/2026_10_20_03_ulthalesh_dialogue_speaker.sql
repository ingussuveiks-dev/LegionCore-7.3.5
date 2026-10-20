-- TALK selects a creature target as the SPEAKER, not the dialogue recipient.
-- Ariden's VICTIM can be a warlock pet: that made TEXT_OVER report the pet's
-- entry instead of 100344, so event 13 never completed the protected-1HP boss.
-- Keep native text groups/timers, optional cast and defeat -> chest -> guide
-- order. Do not change generic TALK's intentional creature-speaker semantics.
UPDATE smart_scripts SET target_type=1
WHERE entryorguid=100344 AND source_type=0 AND action_type=1 AND target_type=2
  AND ((id=1 AND action_param1=1) OR (id=4 AND action_param1=2)
    OR (id IN(11,90) AND action_param1=3));
