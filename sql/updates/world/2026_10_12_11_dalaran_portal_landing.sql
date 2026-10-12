-- In the Blink of an Eye: land on the measured floor beside Khadgar.
-- The previous point (-11099.3, -2211.6, 792) was 34.17 above the city
-- surface and the client retest fell through the floating city.
-- GO 246854 / display 35175 at its existing world rotation has floor
-- Z=747.131 here. Keep map 0, travel credit and the later cinematic intact.
UPDATE `spell_target_position`
SET `target_map`=0, `target_position_x`=-11105, `target_position_y`=-2028,
    `target_position_z`=747.6, `target_orientation`=2.72
WHERE `id`=228327;
