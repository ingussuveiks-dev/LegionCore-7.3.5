-- Keep Genn's original spell_area aura 69486. Phase 190 is the native vermin layer of 70696.
-- Phase 189 is the native Aderic's Repose layer of 70695, whose later spell_area row is retained.
INSERT IGNORE INTO `phase_definitions`
(`zoneId`,`entry`,`phasemask`,`phaseId`,`PreloadMapID`,`VisibleMapID`,`UiWorldMapAreaID`,`flags`,`comment`)
VALUES
(4755,3,2097152,'190',0,0,0,0,'Knee-Deep: vermin on the city side'),
(4714,3,2097152,'190',0,0,0,0,'Knee-Deep: vermin in the tunnel and exit'),
(4714,4,1048576,'189',0,0,0,0,'Aderic''s Repose: Krennan and the next quest offer');

INSERT IGNORE INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,
 `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`Comment`)
VALUES
(13,1,70631,0,0,31,0,3,37889,0,0,'Swing Torch targets Graveyard Rats'),
(13,1,70631,0,1,31,0,3,37891,0,0,'Swing Torch targets Underground Spiders'),
(13,1,70631,0,2,31,0,3,37892,0,0,'Swing Torch targets Putrescent Maggots'),
(23,4755,3,0,0,16,0,2097152,0,0,0,'Worgen city tunnel layer'),
(23,4755,3,0,0,23,0,4758,0,0,0,'Actual city side vermin area'),
(23,4755,3,0,0,9,0,24678,0,0,0,'City vermin during an accepted incomplete travel quest'),
(23,4755,3,0,1,16,0,2097152,0,0,0,'Worgen city tunnel layer'),
(23,4755,3,0,1,23,0,4758,0,0,0,'Actual city side vermin area'),
(23,4755,3,0,1,28,0,24678,0,0,0,'City vermin while the travel quest awaits turn-in'),
(23,4714,3,0,0,16,0,2097152,0,0,0,'Worgen tunnel layer'),
(23,4714,3,0,0,23,0,4790,0,0,0,'Actual middle tunnel'),
(23,4714,3,0,0,9,0,24678,0,0,0,'Middle vermin during an accepted incomplete travel quest'),
(23,4714,3,0,1,16,0,2097152,0,0,0,'Worgen tunnel layer'),
(23,4714,3,0,1,23,0,4790,0,0,0,'Actual middle tunnel'),
(23,4714,3,0,1,28,0,24678,0,0,0,'Middle vermin while the travel quest awaits turn-in'),
(23,4714,3,0,2,16,0,2097152,0,0,0,'Worgen tunnel layer'),
(23,4714,3,0,2,23,0,4727,0,0,0,'Actual tunnel exit'),
(23,4714,3,0,2,9,0,24678,0,0,0,'Exit vermin during an accepted incomplete travel quest'),
(23,4714,3,0,3,16,0,2097152,0,0,0,'Worgen tunnel layer'),
(23,4714,3,0,3,23,0,4727,0,0,0,'Actual tunnel exit'),
(23,4714,3,0,3,28,0,24678,0,0,0,'Exit vermin while the travel quest awaits turn-in'),
(23,4714,4,0,0,16,0,2097152,0,0,0,'Worgen Krennan handoff'),
(23,4714,4,0,0,23,0,4727,0,0,0,'Krennan in Aderic''s Repose'),
(23,4714,4,0,0,9,0,24678,0,0,0,'Krennan during the accepted crossing'),
(23,4714,4,0,1,16,0,2097152,0,0,0,'Worgen Krennan handoff'),
(23,4714,4,0,1,23,0,4727,0,0,0,'Krennan in Aderic''s Repose'),
(23,4714,4,0,1,28,0,24678,0,0,0,'Krennan accepts the completed crossing'),
(23,4714,4,0,2,16,0,2097152,0,0,0,'Worgen Krennan handoff'),
(23,4714,4,0,2,23,0,4727,0,0,0,'Krennan in Aderic''s Repose'),
(23,4714,4,0,2,8,0,24678,0,0,0,'Krennan retains his follow-up offer after the handoff');
