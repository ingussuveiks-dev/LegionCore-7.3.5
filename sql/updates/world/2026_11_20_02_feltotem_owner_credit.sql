-- Feltotem's Fall is a solo Legion artifact challenge. Route these three
-- death actions to the actual invoker's player or controlled-unit owner.
-- Keep both authored boss-death guards and the native 233870 credit spell.
START TRANSACTION;

UPDATE `smart_scripts`
SET `target_type` = 130, `target_param1` = 100, `target_param2` = 0,
    `target_param3` = 0, `target_param4` = 0,
    `action_param2` = `action_param2` | 128,
    `comment` = 'Feltotem finale - invoker player or pet owner casts native credit on self'
WHERE `source_type` = 0 AND `action_type` = 85 AND `action_param1` = 233870
  AND `target_type` = 16
  AND ((`entryorguid` = 117230 AND `id` = 7 AND `event_type` = 61)
    OR (`entryorguid` = 117484 AND `id` = 14 AND `event_type` = 6));

UPDATE `smart_scripts`
SET `target_type` = 130, `target_param1` = 100, `target_param2` = 0,
    `target_param3` = 0, `target_param4` = 0,
    `comment` = 'Jormog death - record scenario criterion for the invoker player or pet owner'
WHERE `source_type` = 0 AND `entryorguid` = 117484 AND `id` = 15
  AND `event_type` = 6 AND `action_type` = 205 AND `action_param1` = 92
  AND `action_param2` = 58088 AND `target_type` = 16;

-- ElseGroup 3 duplicated Warlock. The existing Discipline specialization
-- filter already admits spec 256 in Victoria's Feltotem teleport actions.
UPDATE `conditions`
SET `ConditionValue1` = 16,
    `Comment` = 'Feltotem challenge quest 45627 - allow Priest after quest 45842'
WHERE `SourceTypeOrReferenceId` = 19 AND `SourceGroup` = 0
  AND `SourceEntry` = 45627 AND `SourceId` = 0 AND `ElseGroup` = 3
  AND `ConditionTypeOrReference` = 15 AND `ConditionTarget` = 0
  AND `ConditionValue1` = 256 AND `ConditionValue2` = 0
  AND `ConditionValue3` = 0 AND `NegativeCondition` = 0;

COMMIT;
