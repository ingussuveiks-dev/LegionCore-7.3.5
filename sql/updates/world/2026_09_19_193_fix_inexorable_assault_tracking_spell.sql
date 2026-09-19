-- Area-trigger actions are validated against SpellInfo even for NO_ACTION.
-- Use the owning passive as the tracking row's valid spell reference.
UPDATE `areatrigger_actions`
SET `spellId` = 253593
WHERE `entry` = 11655 AND `id` = 0 AND `customEntry` = 0;
