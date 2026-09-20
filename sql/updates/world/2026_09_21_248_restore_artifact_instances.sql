-- Bind existing artifact scenario content to its instance scripts.
UPDATE `instance_template`
SET `script` = 'instance_monk_intro'
WHERE `map` = 1014;

UPDATE `instance_template`
SET `script` = 'instance_violethold_mage'
WHERE `map` = 1494;

UPDATE `instance_template`
SET `script` = 'instance_sheylun'
WHERE `map` = 1541;

UPDATE `instance_template`
SET `script` = 'instance_dreadblades'
WHERE `map` = 1545;
