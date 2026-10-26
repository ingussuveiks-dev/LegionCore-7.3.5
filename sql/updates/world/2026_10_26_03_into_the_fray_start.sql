START TRANSACTION;

-- 38834 is a travel/turn-in quest with no objectives. The legacy start
-- script contains seven identical QUEST_EXPLORED commands. Clear that forced
-- completion path and its obsolete exploration requirement; ordinary native
-- acceptance still makes an objective-free quest ready for its real ender.
SET @into_fray_start := EXISTS (SELECT 1 FROM quest_template WHERE ID=38834 AND StartScript=38834 AND CompleteScript=0)
AND EXISTS (SELECT 1 FROM quest_template_addon WHERE ID=38834 AND (SpecialFlags & 2)=2)
AND NOT EXISTS (SELECT 1 FROM quest_objectives WHERE QuestID=38834)
AND NOT EXISTS (SELECT 1 FROM quest_template WHERE ID<>38834 AND (StartScript=38834 OR CompleteScript=38834))
AND (SELECT COUNT(*) FROM quest_start_scripts WHERE id=38834)=7
AND (SELECT COUNT(*) FROM quest_start_scripts WHERE id=38834 AND delay=0 AND command=7 AND datalong=38834
    AND datalong2=0 AND dataint=0 AND x=0 AND y=0 AND z=0 AND o=0)=7
AND NOT EXISTS (SELECT 1 FROM quest_end_scripts WHERE id=38834);

DELETE FROM quest_start_scripts WHERE id=38834 AND @into_fray_start;
UPDATE quest_template SET StartScript=0 WHERE ID=38834 AND @into_fray_start;
UPDATE quest_template_addon SET SpecialFlags=SpecialFlags & ~2 WHERE ID=38834 AND @into_fray_start;

COMMIT;
