-- Restore Auchenai Crypts instance state so boss progress is persisted and
-- dungeon encounter credit can accumulate across both encounters.
UPDATE `instance_template`
SET `script` = 'instance_auchenai_crypts'
WHERE `map` = 558;
