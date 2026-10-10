-- Previous scripts awarded these objectives automatically. Preserve rewarded
-- quests, but do not treat unturned-in shortcut credit as a completed event.
START TRANSACTION;
DELETE o FROM character_queststatus_objectives o
JOIN character_queststatus q ON q.guid=o.guid AND q.quest=o.quest
LEFT JOIN character_queststatus_rewarded r ON r.guid=q.guid AND r.quest=q.quest
WHERE r.guid IS NULL AND q.status IN(1,3) AND
 ((q.quest IN(38687,41763) AND o.objective IN(0,1,2,3,5,6,7)) OR (q.quest=38743 AND o.objective=0));
UPDATE character_queststatus q
LEFT JOIN character_queststatus_rewarded r ON r.guid=q.guid AND r.quest=q.quest
SET q.status=3 WHERE r.guid IS NULL AND q.status=1 AND q.quest IN(38687,41763,38743);
COMMIT;
