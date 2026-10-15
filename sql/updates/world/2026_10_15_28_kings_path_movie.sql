-- EventObject scripts use source 13. Source 10 is a Scene script and left this
-- outdoor trigger unconditional, including during Ritual Ruination (43254).
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1
 AND SourceEntry=372 AND SourceId=10 AND ConditionTypeOrReference=9 AND ConditionValue1=46282;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1
 AND SourceEntry=372 AND SourceId=13 AND ConditionTypeOrReference IN(9,41)
 AND ConditionValue1=46282;
INSERT INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,
 ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,
 ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES
 (22,1,372,13,0,9,0,46282,0,0,0,0,'','The King''s Path movie: active incomplete quest only'),
 (22,1,372,13,0,41,0,46282,119426,1,1,0,'','The King''s Path movie: do not replay credited arrival');
