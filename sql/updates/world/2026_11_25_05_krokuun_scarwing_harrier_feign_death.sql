-- Scarwing Harrier is a living flyer with credit for Overwhelming Power.
-- Its default addon applies permanent feign death and prevents normal pursuit.
UPDATE `creature_template_addon`
SET `auras` = TRIM(REGEXP_REPLACE(`auras`, '(^| )159474(?= |$)', ''))
WHERE `entry` = 119604
  AND CONCAT(' ', `auras`, ' ') LIKE '% 159474 %';
