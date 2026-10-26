START TRANSACTION;

-- 189764 legitimately completes this event quest. Keep its required event flag.
-- Reconcile both the original seven-command data and the superseded migration03
-- state, retaining one actual command7 event for the modernized native caller.
SET @into_fray_event := EXISTS (SELECT 1 FROM quest_template WHERE ID=38834 AND StartScript IN (0,38834) AND CompleteScript=0)
AND EXISTS (SELECT 1 FROM quest_template_addon WHERE ID=38834)
AND NOT EXISTS (SELECT 1 FROM quest_objectives WHERE QuestID=38834)
AND NOT EXISTS (SELECT 1 FROM quest_template WHERE ID<>38834 AND (StartScript=38834 OR CompleteScript=38834))
AND (SELECT COUNT(*) FROM quest_start_scripts WHERE id=38834) IN (0,1,7)
AND NOT EXISTS (SELECT 1 FROM quest_start_scripts WHERE id=38834 AND NOT (delay=0 AND command=7 AND datalong=38834
    AND datalong2=0 AND dataint=0 AND x=0 AND y=0 AND z=0 AND o=0))
AND NOT EXISTS (SELECT 1 FROM quest_end_scripts WHERE id=38834);

DELETE FROM quest_start_scripts WHERE id=38834 AND @into_fray_event;
INSERT INTO quest_start_scripts (id,delay,command,datalong,datalong2,dataint,x,y,z,o)
SELECT 38834,0,7,38834,0,0,0,0,0,0 WHERE @into_fray_event;
UPDATE quest_template SET StartScript=38834 WHERE ID=38834 AND @into_fray_event;
UPDATE quest_template_addon SET SpecialFlags=SpecialFlags | 2 WHERE ID=38834 AND @into_fray_event;

COMMIT;
