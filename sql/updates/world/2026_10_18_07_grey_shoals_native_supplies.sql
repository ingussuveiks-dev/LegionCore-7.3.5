-- Restore the Kvaldir Weapon Rack identified by the Legion guide and native
-- 229972 ->115739 credit. Public TC build27843 corroborates entry266170/display35575;
-- this template already exists here with VerifiedBuild23222.
-- Keep the four reconstructed positions: exact retail coordinates are unknown.
UPDATE `gameobject_template`
SET `ScriptName`=''
WHERE `entry` IN(241741,244704) AND `ScriptName`='go_grey_shoals_supplies';

UPDATE `gameobject_template`
SET `name`='Kvaldir Weapon Rack', `castBarCaption`='Destroying',
    `ScriptName`='go_grey_shoals_supplies',
    -- This core truncates millisecond autoClose to whole seconds. A positive
    -- 1ms value becomes0 and selects GO::Use's unrelated60-second fallback.
    -- Round this scoped template up to the smallest representable delay.
    `Data3`=IF(`Data3`=1,1000,`Data3`)
WHERE `entry`=266170 AND `type`=10 AND `displayId`=35575 AND `Data10`=229972
  AND `ScriptName` IN('','go_grey_shoals_supplies');

UPDATE `gameobject`
SET `id`=266170
WHERE `guid` BETWEEN 147040012 AND 147040015
  AND `id` IN(241741,244704) AND `map`=1220 AND `areaId`=8297
  AND EXISTS(SELECT 1 FROM `gameobject_template` WHERE `entry`=266170
      AND `type`=10 AND `displayId`=35575 AND `Data10`=229972
      AND `ScriptName`='go_grey_shoals_supplies');
