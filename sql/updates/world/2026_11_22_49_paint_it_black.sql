-- Stop the Sentry-Bot's active attack when Blind Sentry switches it to passive.
-- Keep the existing dialogue, quest credit and three-second despawn links.
UPDATE `smart_scripts`
SET `action_param2` = 1, `comment` = 'Crushcog Sentry-Bot - On Blind Sentry - Go Passive and Stop Attacking'
WHERE `entryorguid` = 42291 AND `source_type` = 0 AND `id` = 2 AND `link` = 3
  AND `event_type` = 61 AND `action_type` = 8 AND `action_param1` = 0 AND `action_param2` = 0;
