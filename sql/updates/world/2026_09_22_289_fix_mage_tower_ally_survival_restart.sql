-- End of the Risen Threat (scenario 1281) exposes criteria event 56912 as
-- "Your allies must survive" on several ANY branches.  It is a survival
-- guard, not an alternate step-completion event: firing it as criteria credit
-- would let one event advance or skip multiple healer challenge stages.
--
-- Enforce the guard through the encounter's intended Restart spell instead.
-- Use a living nearby player as both caster and target so the restart remains
-- reliable after the protected ally has already entered the dead state.
UPDATE `smart_scripts`
SET `action_type` = 86,
    `action_param1` = 236132,
    `action_param2` = 0,
    `action_param3` = 21,
    `action_param4` = 100,
    `action_param5` = 0,
    `action_param6` = 0,
    `target_type` = 21,
    `target_param1` = 100,
    `target_param2` = 0,
    `target_param3` = 0,
    `comment` = 'On death - nearest player casts Restart on self (Mage Tower ally survival)'
WHERE `source_type` = 0
  AND `entryorguid` IN (118447, 118448, 118451)
  AND `event_type` = 6
  AND `action_param1` = 236132;
