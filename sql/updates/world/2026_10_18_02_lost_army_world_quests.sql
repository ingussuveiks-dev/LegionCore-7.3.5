-- The Lost Army: outdoor fragments belong to an active Helarjar landing.
-- Keep the existing per-creature drop chances/counts and native quest-item check.
DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId`=1 AND `SourceGroup` IN (115025,115031,115751,115846,116180,116600,116601)
  AND `SourceEntry`=142487 AND `SourceId`=0 AND `ConditionTypeOrReference`=60;

INSERT INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(1,115025,142487,0,0,60,0,44737,0,0,0,0,'','Lost Army - Helarjar Raider - Grey Shoals active'),
(1,115031,142487,0,0,60,0,44737,0,0,0,0,'','Lost Army - Cursed Rigger - Grey Shoals active'),
(1,115751,142487,0,0,60,0,44737,0,0,0,0,'','Lost Army - Seirdr - Grey Shoals active'),
(1,115846,142487,0,0,60,0,44923,0,0,0,0,'','Lost Army - Vaettfang - Vrekt active'),
(1,116180,142487,0,0,60,0,44923,0,0,0,0,'','Lost Army - Mist Seer - Vrekt active'),
(1,116600,142487,0,0,60,0,44923,0,0,0,0,'','Lost Army - Cursed Sea Dog - Vrekt active'),
(1,116601,142487,0,0,60,0,44923,0,0,0,0,'','Lost Army - Helarjar Marauder - Vrekt active');
