-- Restore the two SmartAI Nightborne Boat paths referenced by creature 105264.
-- The coordinates are identical in the 2020 and 2024 LegionCore 7.3.5 data.
INSERT IGNORE INTO `waypoints`
    (`entry`, `pointid`, `position_x`, `position_y`, `position_z`, `point_comment`)
VALUES
    (9100402, 1,  967.552, 3814.90, -0.000107709, 'Nightborne Boat path 9100402'),
    (9100402, 2,  972.879, 3807.10, -0.000107709, 'Nightborne Boat path 9100402'),
    (9100402, 3,  984.357, 3792.25, -0.000107709, 'Nightborne Boat path 9100402'),
    (9100402, 4,  991.347, 3779.85, -0.000107709, 'Nightborne Boat path 9100402'),
    (9100402, 5, 1007.580, 3778.13,  0.000370026, 'Nightborne Boat path 9100402'),
    (9100403, 1, 1007.330, 3777.40,  0.000280395, 'Nightborne Boat path 9100403'),
    (9100403, 2, 1022.070, 3762.41,  0.000280395, 'Nightborne Boat path 9100403'),
    (9100403, 3, 1018.210, 3741.99,  0.000280395, 'Nightborne Boat path 9100403'),
    (9100403, 4, 1010.020, 3725.37,  0.000280395, 'Nightborne Boat path 9100403'),
    (9100403, 5, 1005.500, 3696.37,  0.000280395, 'Nightborne Boat path 9100403'),
    (9100403, 6, 1010.360, 3661.59,  0.000280395, 'Nightborne Boat path 9100403'),
    (9100403, 7, 1010.660, 3635.03,  0.000280395, 'Nightborne Boat path 9100403'),
    (9100403, 8, 1004.290, 3610.06,  0.000280395, 'Nightborne Boat path 9100403'),
    (9100403, 9,  991.291, 3576.66,  0.000280395, 'Nightborne Boat path 9100403'),
    (9100403,10,  977.361, 3558.93,  0.000280395, 'Nightborne Boat path 9100403'),
    (9100403,11,  974.691, 3536.38,  0.000280395, 'Nightborne Boat path 9100403'),
    (9100403,12,  975.415, 3504.35,  0.000280395, 'Nightborne Boat path 9100403'),
    (9100403,13,  970.645, 3478.13,  0.000280395, 'Nightborne Boat path 9100403');
