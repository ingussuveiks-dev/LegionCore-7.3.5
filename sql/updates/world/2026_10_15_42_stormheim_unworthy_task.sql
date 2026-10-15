-- An Unworthy Task: restore TDB735 cage interaction, not acceptance completion.
-- Native objective280004 stays GO241778/storage1/amount1. Preserve all phases,
-- Colborn variants and both parallel follow-ups38324/38339.
UPDATE `gameobject_template`
SET `type`=10,
    `Data0`=1691,
    `Data1`=39837,
    `Data2`=0,
    `Data3`=3000,
    `Data4`=0,
    `Data5`=0,
    `Data6`=0,
    `Data7`=0,
    `Data8`=0,
    `Data9`=0,
    `Data10`=0,
    `Data11`=0,
    `Data12`=0,
    `Data13`=0,
    `Data14`=21400,
    `Data15`=0,
    `Data16`=0,
    `Data17`=0,
    `Data18`=0,
    `Data19`=0,
    `Data20`=1,
    `Data21`=0,
    `Data22`=0,
    `Data23`=0,
    `Data24`=0,
    `Data25`=0,
    `Data26`=1,
    `Data27`=0,
    `Data28`=0,
    `Data29`=0,
    `Data30`=0,
    `Data31`=0,
    `Data32`=0
WHERE `entry`=241778 AND `type`=0 AND `displayId`=7469
  AND `Data0`=0 AND `Data1`=1690 AND `Data2`=3000
  AND `Data7`=35984 AND `AIName`='' AND `ScriptName`='';

UPDATE `quest_template` SET `StartScript`=0 WHERE `ID`=39837 AND `StartScript`=39837;
-- The obsolete event requirement blocks CanCompleteQuest after real GO credit.
UPDATE `quest_template_addon` a JOIN `quest_template` q ON q.`ID`=a.`ID`
SET a.`SpecialFlags`=a.`SpecialFlags` & ~2 WHERE a.`ID`=39837 AND q.`StartScript`=0;
DELETE FROM `quest_start_scripts`
WHERE `id`=39837 AND `delay`=0 AND `command`=7 AND `datalong`=39837
  AND `datalong2`=0 AND `dataint`=0 AND `x`=0 AND `y`=0 AND `z`=0 AND `o`=0
  AND NOT EXISTS (SELECT 1 FROM `quest_template` WHERE `StartScript`=39837);
