-- Spell 113942 is the Legion Demonic Gateway reuse debuff.  The removed
-- positive aura check made activation impossible because spell 135557 does
-- not exist in the 7.3.5 client data.
UPDATE `areatrigger_actions`
SET `hasAura` = -113942
WHERE `entry` = 197
  AND `customEntry` = 131
  AND `spellId` = 113902
  AND `hasAura` = 135557;
