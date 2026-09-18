-- An old scenario workaround changed Bloodwing's default passenger to entry 0
-- to mean "no passenger".  The loader has never treated 0 as a sentinel: it
-- reports the row and skips it, which is functionally the same as no row.
CREATE TABLE IF NOT EXISTS `_backup_20260918_vehicle_accessory_placeholder`
LIKE `vehicle_template_accessory`;

REPLACE INTO `_backup_20260918_vehicle_accessory_placeholder`
SELECT *
FROM `vehicle_template_accessory`
WHERE `EntryOrAura` = 94517
  AND `seat_id` = 0
  AND `accessory_entry` = 0;

DELETE FROM `vehicle_template_accessory`
WHERE `EntryOrAura` = 94517
  AND `seat_id` = 0
  AND `accessory_entry` = 0;
