-- Remap the pre-release objective counters to 7.3.5 slots. Applied once by
-- the character updater, together with world 2026_10_10_01_valsharah_chain.
-- 38582's shared old counter cannot prove a boss kill: keep plant progress
-- in slot 2 and leave the newly separated boss slot 0 uncredited.
START TRANSACTION;
-- The former shared plant/boss counter cannot establish which action occurred.
-- Preserve all plant progress and completed rewards; active quests must kill
-- the boss again to establish the now-independent boss objective.
UPDATE character_queststatus s SET s.status=3 WHERE s.quest=38582 AND s.status=1
AND NOT EXISTS (SELECT 1 FROM character_queststatus_rewarded r WHERE r.guid=s.guid AND r.quest=s.quest);
CREATE TEMPORARY TABLE valsharah_saved_objectives LIKE character_queststatus_objectives;
INSERT INTO valsharah_saved_objectives
SELECT * FROM character_queststatus_objectives
WHERE (quest=39384 AND objective IN(0,1)) OR (quest=40573 AND objective IN(0,3));
DELETE FROM character_queststatus_objectives
WHERE (quest=39384 AND objective IN(0,1)) OR (quest=40573 AND objective IN(0,3));
INSERT INTO character_queststatus_objectives (guid,account,quest,objective,data)
SELECT guid,account,quest,
 CASE WHEN quest=39384 AND objective=0 THEN 3 WHEN quest=39384 AND objective=1 THEN 0
      WHEN quest=40573 AND objective=0 THEN 5 WHEN quest=40573 AND objective=3 THEN 4 END,data
FROM valsharah_saved_objectives
ON DUPLICATE KEY UPDATE data=GREATEST(character_queststatus_objectives.data,VALUES(data));
DROP TEMPORARY TABLE valsharah_saved_objectives;
COMMIT;
