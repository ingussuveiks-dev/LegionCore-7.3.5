-- Preserve the two creature combat rows before replacing removed spell IDs.
CREATE TABLE IF NOT EXISTS `_backup_20260918_boar_sea_dog_spells` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_boar_sea_dog_spells`
SELECT *
FROM `smart_scripts`
WHERE (`entryorguid` = 34647 AND `source_type` = 0 AND `id` = 0 AND `action_type` = 11 AND `action_param1` = 35290)
   OR (`entryorguid` = 45196 AND `source_type` = 0 AND `id` = 4 AND `action_type` = 11 AND `action_param1` = 33408);

-- Barrens Boar: the old hunter-pet Gore was removed; use the valid NPC Gore
-- already used by many creature SmartAI scripts in this database.
UPDATE `smart_scripts`
SET `action_param1` = 32019
WHERE `entryorguid` = 34647 AND `source_type` = 0 AND `id` = 0
  AND `action_type` = 11 AND `action_param1` = 35290;

-- Orc Sea Dog: replace the removed Sinister Strike with the normalized weapon-
-- damage NPC version used by Cataclysm-era creatures, including nearby content.
UPDATE `smart_scripts`
SET `action_param1` = 60195
WHERE `entryorguid` = 45196 AND `source_type` = 0 AND `id` = 4
  AND `action_type` = 11 AND `action_param1` = 33408;
