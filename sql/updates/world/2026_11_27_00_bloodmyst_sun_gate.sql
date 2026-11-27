-- The shared Sun Gate stays locked until the four original controllers are down.
UPDATE `gameobject_template`
SET `ScriptName`='go_bloodmyst_sun_gate', `flags`=`flags` | 16
WHERE `entry`=182026 AND `type`=10 AND `Data1`=9740 AND `ScriptName` IN ('','go_bloodmyst_sun_gate');

-- Build 26972 has no 38096 effects. The activated controller now deactivates through its GO lifecycle.
UPDATE `gameobject_template`
SET `ScriptName`='go_bloodmyst_sun_gate', `Data10`=0
WHERE `entry`=184850 AND `type`=10 AND `Data1`=9740 AND `Data5`=1
  AND `Data10` IN (0,38096) AND `ScriptName` IN ('','go_bloodmyst_sun_gate');
