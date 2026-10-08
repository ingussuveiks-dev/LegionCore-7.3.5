-- Charge categories were only held in Player memory and reset on reconnect.
-- Millisecond timestamps preserve partial recovery and allow offline recovery.
CREATE TABLE IF NOT EXISTS `character_spell_charges` (
  `guid` bigint unsigned NOT NULL,
  `category` int unsigned NOT NULL,
  `spell` int unsigned NOT NULL,
  `consumedCharges` tinyint unsigned NOT NULL,
  `nextRecoveryTime` bigint unsigned NOT NULL,
  `chargeRegenTime` int unsigned NOT NULL,
  PRIMARY KEY (`guid`, `category`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
