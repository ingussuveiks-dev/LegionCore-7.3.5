-- In conversation 4887, 32842 ms is the full timeline, shared by all four actors.
-- The 7.3.5 loader adds actor durations. Keep that timeline on its first actor.
UPDATE `conversation_actor` AS actor
JOIN `conversation_actor` AS first_actor
  ON first_actor.`entry` = actor.`entry` AND first_actor.`id` = 0
SET actor.`duration` = 0
WHERE actor.`entry` = 4887
  AND first_actor.`actorId` = 57485 AND first_actor.`duration` = 32842
  AND (actor.`id`, actor.`actorId`) IN ((1, 57481), (2, 57480), (3, 58065))
  AND actor.`duration` = 32842;
