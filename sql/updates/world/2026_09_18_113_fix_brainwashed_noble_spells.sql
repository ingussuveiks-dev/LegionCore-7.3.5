-- Preserve all affected Brainwashed Noble rows. The imported caster template used
-- Arcane Charge (a self aura) as its projectile and a pre-Legion Mage Armor spell.
CREATE TABLE IF NOT EXISTS `_backup_20260918_brainwashed_noble_spells` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_brainwashed_noble_spells`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 596 AND `source_type` = 0
  AND ((`id` IN (2, 3) AND `action_type` = 11 AND `action_param1` = 36032)
    OR (`id` = 14 AND `action_type` = 11 AND `action_param1` = 6117));

-- Arcane Blast is the actual hostile spell used by the reference creature script.
UPDATE `smart_scripts`
SET `action_param1` = 20883,
    `comment` = IF(`id` = 2, 'Cast Arcane Blast on Aggro', 'Cast Arcane Blast')
WHERE `entryorguid` = 596 AND `source_type` = 0 AND `id` IN (2, 3)
  AND `action_type` = 11 AND `action_param1` = 36032;

-- Player Mage Armor 6117 was removed before Legion. The remaining spells named
-- Mage Armor in 7.3.5 are unrelated encounter/NPC auras, so do not substitute one.
DELETE FROM `smart_scripts`
WHERE `entryorguid` = 596 AND `source_type` = 0 AND `id` = 14
  AND `action_type` = 11 AND `action_param1` = 6117;
