-- The script_texts loader indexes rows globally by entry, although the table's
-- primary key also includes npc_entry.  These later rows therefore lose their
-- collision and are ignored.  Their scripts have already been migrated to
-- creature_text/Talk(), while the winning legacy or generic rows are still
-- referenced by DoScriptText and must remain unchanged.
CREATE TABLE IF NOT EXISTS `_backup_20260918_duplicate_script_texts`
LIKE `script_texts`;

REPLACE INTO `_backup_20260918_duplicate_script_texts`
SELECT *
FROM `script_texts`
WHERE (`npc_entry` = 15420 AND `entry` IN (-1000210, -1000209))
   OR (`npc_entry` = 27655 AND `entry` IN
       (-1578016, -1578015, -1578014, -1578013, -1578012,
        -1578004, -1578003, -1578002, -1578001, -1578000))
   OR (`npc_entry` = 29032 AND `entry` BETWEEN -1609087 AND -1609080)
   OR (`npc_entry` = 49869 AND `entry` BETWEEN -1000005 AND -1000001);

DELETE FROM `script_texts`
WHERE (`npc_entry` = 15420 AND `entry` IN (-1000210, -1000209))
   OR (`npc_entry` = 27655 AND `entry` IN
       (-1578016, -1578015, -1578014, -1578013, -1578012,
        -1578004, -1578003, -1578002, -1578001, -1578000))
   OR (`npc_entry` = 29032 AND `entry` BETWEEN -1609087 AND -1609080)
   OR (`npc_entry` = 49869 AND `entry` BETWEEN -1000005 AND -1000001);
