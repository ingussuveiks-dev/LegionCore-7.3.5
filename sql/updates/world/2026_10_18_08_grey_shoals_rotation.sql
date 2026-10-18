-- Preserve the native SmartAI dispatcher/events while adding the outdoor
-- rotation lifecycle. The C++ factory excludes summons and other maps/areas.
UPDATE `creature_template`
SET `ScriptName`='npc_grey_shoals'
WHERE `entry` IN(115025,115026,115031,115751)
  AND `AIName`='SmartAI' AND `ScriptName` IN('','npc_grey_shoals');
