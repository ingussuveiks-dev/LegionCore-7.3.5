-- Before commit b4d1807 this column was a bool named move_flag. A legacy value
-- of 15 therefore meant true/run. The schema migration renamed the column but
-- did not normalize nonzero bool values to the new RUN enum value (1).
UPDATE `waypoint_data`
SET `move_type` = 1
WHERE `id` IN (501874, 501877)
  AND `move_type` = 15;
