-- QuestData treats UINT64_MAX as unrestricted. These custom quests used the
-- column default 0, so the loader replaced it with UINT64_MAX on every start.
UPDATE `quest_template`
SET `AllowableRaces` = 18446744073709551615
WHERE `ID` BETWEEN 60010 AND 60030
  AND `AllowableRaces` = 0;
