-- Defiled Consecration's ground trigger (202703) lasts 8 seconds, while its
-- damage aura (202704) is permanent. Remove the aura both when a unit leaves
-- and when the trigger expires or is otherwise removed. AreaTrigger::Remove
-- dispatches REMOVE (32) before clearing its affected targets.
UPDATE `areatrigger_actions`
SET `moment` = `moment` | 32
WHERE `entry` = 5856 AND `customEntry` = 0 AND `id` = 1
    AND `actionType` = 1 AND `targetFlags` = 4096 AND `spellId` = 202704
    AND `moment` = 2;
