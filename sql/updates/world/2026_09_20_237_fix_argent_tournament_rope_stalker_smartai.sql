-- Restore the eight Argent Tournament rope-beam stalker SmartAI bindings.
-- TrinityCore's original 3.3.5 data places NPC 25171 at these eight positions
-- around Gormok and Icehowl. The spawns were renumbered to GUIDs 137150-137157
-- in this world database, but their per-spawn SmartAI still used the obsolete
-- GUIDs 199025-199032.
START TRANSACTION;

UPDATE `smart_scripts`
SET `entryorguid` = CASE `entryorguid`
    WHEN -199025 THEN -137150
    WHEN -199026 THEN -137151
    WHEN -199027 THEN -137152
    WHEN -199028 THEN -137153
    WHEN -199029 THEN -137154
    WHEN -199030 THEN -137155
    WHEN -199031 THEN -137156
    WHEN -199032 THEN -137157
    ELSE `entryorguid`
END
WHERE `source_type`=0
  AND `entryorguid` IN (-199025,-199026,-199027,-199028,-199029,-199030,-199031,-199032);

COMMIT;
