-- Eight Lesser Val'kyr spawns override the template with display 26101,
-- absent from 7.3.5 CreatureDisplayInfo. It also has no creature_model_info.
-- Use native template selection (valid displays 25517/33997) instead.
UPDATE `creature` SET `modelid` = 0
WHERE `id` = 45239 AND `modelid` = 26101;
