-- The Ritual Focus has spell-click actions, but its inherited unit flag blocks client selection.
UPDATE `creature_template`
SET `unit_flags` = `unit_flags` & ~33554432
WHERE `entry` = 108855 AND (`npcflag` & 16777216) <> 0 AND (`unit_flags` & 33554432) <> 0;
