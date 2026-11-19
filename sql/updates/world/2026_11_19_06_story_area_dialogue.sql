-- These single-action outdoor dialogue roots use EventObject SmartScripts13.
-- Retain their distinct active/complete/not-started quest gates and ElseGroups.
UPDATE conditions SET SourceId=13
WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceId=10
AND SourceEntry IN (127,237,238,305,321,366,515,530,532);
