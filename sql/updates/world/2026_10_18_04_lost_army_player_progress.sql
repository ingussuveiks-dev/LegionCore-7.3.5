-- Rotation alone must not allow fragments after the player's landing ends.
-- Vrekt also stops at the ten-Helarjar quota while Vaettfang is outstanding.
-- A raid group must not bypass this quota by suppressing outdoor quest credit.
-- Preserve source1 rotation60, native fragment eligibility and all loot rates.
DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId`=1 AND `SourceEntry`=142487 AND `SourceId`=0 AND `ElseGroup`=0
  AND ((`SourceGroup` IN (115025,115031,115751) AND `ConditionTypeOrReference`=9 AND `ConditionValue1`=44737)
    OR (`SourceGroup` IN (115846,116180,116600,116601) AND `ConditionValue1`=44923
      AND (`ConditionTypeOrReference`=9 OR (`ConditionTypeOrReference`=41 AND `ConditionValue2`=115845)))
    OR (`SourceGroup` IN (115025,115031,115751,115846,116180,116600,116601)
      AND `ConditionTypeOrReference`=58 AND `ConditionValue1`=0 AND `ConditionValue2`=1 AND `ConditionValue3`=0));

INSERT INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(1,115025,142487,0,0,9,0,44737,0,0,0,0,'','Lost Army - Raider - player Grey Shoals unfinished'),
(1,115031,142487,0,0,9,0,44737,0,0,0,0,'','Lost Army - Rigger - player Grey Shoals unfinished'),
(1,115751,142487,0,0,9,0,44737,0,0,0,0,'','Lost Army - Seirdr - player Grey Shoals unfinished'),
(1,115846,142487,0,0,9,0,44923,0,0,0,0,'','Lost Army - Vaettfang - player Vrekt unfinished'),
(1,116180,142487,0,0,9,0,44923,0,0,0,0,'','Lost Army - Mist Seer - player Vrekt unfinished'),
(1,116600,142487,0,0,9,0,44923,0,0,0,0,'','Lost Army - Sea Dog - player Vrekt unfinished'),
(1,116601,142487,0,0,9,0,44923,0,0,0,0,'','Lost Army - Marauder - player Vrekt unfinished'),
(1,115025,142487,0,0,58,0,0,1,0,0,0,'','Lost Army - Raider - not in raid group'),
(1,115031,142487,0,0,58,0,0,1,0,0,0,'','Lost Army - Rigger - not in raid group'),
(1,115751,142487,0,0,58,0,0,1,0,0,0,'','Lost Army - Seirdr - not in raid group'),
(1,115846,142487,0,0,58,0,0,1,0,0,0,'','Lost Army - Vaettfang - not in raid group'),
(1,116180,142487,0,0,58,0,0,1,0,0,0,'','Lost Army - Mist Seer - not in raid group'),
(1,116600,142487,0,0,58,0,0,1,0,0,0,'','Lost Army - Sea Dog - not in raid group'),
(1,116601,142487,0,0,58,0,0,1,0,0,0,'','Lost Army - Marauder - not in raid group'),
(1,115846,142487,0,0,41,0,44923,115845,10,1,0,'','Lost Army - Vaettfang - ten Helarjar quota not reached'),
(1,116180,142487,0,0,41,0,44923,115845,10,1,0,'','Lost Army - Mist Seer - ten Helarjar quota not reached'),
(1,116600,142487,0,0,41,0,44923,115845,10,1,0,'','Lost Army - Sea Dog - ten Helarjar quota not reached'),
(1,116601,142487,0,0,41,0,44923,115845,10,1,0,'','Lost Army - Marauder - ten Helarjar quota not reached');
