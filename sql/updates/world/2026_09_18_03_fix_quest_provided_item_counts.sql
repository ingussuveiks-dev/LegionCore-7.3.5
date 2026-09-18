-- These quests provide an existing StartItem but had no addon row, leaving the
-- provided count at 0. The loader already falls back to one item at runtime.
INSERT INTO `quest_template_addon` (`ID`, `ProvidedItemCount`)
SELECT `q`.`ID`, 1
FROM `quest_template` AS `q`
LEFT JOIN `quest_template_addon` AS `a` ON `a`.`ID` = `q`.`ID`
WHERE `q`.`ID` IN
(
    7061, 45061, 45405, 45755, 45758, 45759, 45760, 45761, 45762,
    47745, 47750, 48027, 48028, 48029, 48034, 48035, 48036, 48037,
    48038, 48039, 48040, 48041, 48042, 48056, 48065, 48954, 49077,
    49813, 49846, 49860, 49864, 50337, 50338
)
  AND `q`.`StartItem` <> 0
  AND `a`.`ID` IS NULL;
