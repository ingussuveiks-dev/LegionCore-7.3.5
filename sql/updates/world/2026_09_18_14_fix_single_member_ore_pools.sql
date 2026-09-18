-- These two child pools each contain only one Rich Cobalt Deposit.  The 10%
-- value was left from the usual 90/10 normal/rich pair, so each pool failed
-- validation and never spawned.  The mother pool still selects only 30 of its
-- 100 spawn-point children; a selected single-member child must total 100%.
UPDATE `pool_gameobject`
SET `chance` = 100
WHERE (`guid` = 44625 AND `pool_entry` = 10150 AND `chance` = 10)
   OR (`guid` = 44628 AND `pool_entry` = 10153 AND `chance` = 10);
