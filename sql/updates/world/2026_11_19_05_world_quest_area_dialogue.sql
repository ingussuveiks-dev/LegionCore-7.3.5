-- Outdoor world-quest introductions dispatch EventObject SmartScripts (13),
-- not Scene SmartScripts (10). Keep their authored active-quest conditions.
UPDATE conditions SET SourceId=13
WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceId=10
AND SourceEntry IN
(200,203,204,205,208,209,210,211,212,213,214,215,217,218,219,220,
 221,222,223,224,225,226,227,229,230,231,232,233,307,308,331,332);
