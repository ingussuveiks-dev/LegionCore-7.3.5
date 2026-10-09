-- The old Orbyth accessory auto-created Ael'Yith as a vehicle-owned rider.
-- The recovery script now creates the player's second stage after Orbyth's
-- actual kill. Remove only the obsolete automatic rider link, avoiding a
-- duplicate enemy and an accessory-loader dependency on player spellclick.
DELETE FROM vehicle_template_accessory
WHERE EntryOrAura=91155 AND accessory_entry=108721 AND seat_id=0;
