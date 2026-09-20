-- Restore Dire Maul instance/LFG state tracking.
UPDATE `instance_template`
SET `script` = 'instance_dire_maul'
WHERE `map` = 429;
