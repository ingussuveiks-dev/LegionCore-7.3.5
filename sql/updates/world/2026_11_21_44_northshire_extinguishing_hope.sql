-- Keep the item-driven periodic controller inside the vineyards during the unfinished quest.
UPDATE `spell_area` SET `quest_start` = 26391, `quest_start_status` = 8
WHERE `spell` = 80199 AND `area` = 59 AND `quest_start` = 0 AND `quest_end` = 0 AND `autocast` = 0;

INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
     `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
VALUES
    (13, 1, 80208, 0, 0, 31, 0, 3, 42940, 0, 0, 0, '', 'Spray Water selects vineyard fires'),
    (17, 0, 80199, 0, 0, 2, 0, 58362, 1, 0, 0, 0, '', 'Extinguisher use requires the carried extinguisher'),
    (17, 0, 80208, 0, 0, 1, 0, 80199, 0, 0, 0, 0, '', 'Spray Water requires the periodic extinguisher effect'),
    (22, 3, 42940, 0, 0, 9, 0, 26391, 0, 0, 0, 0, '', 'Fire credit requires an unfinished Extinguishing Hope'),
    (22, 3, 42940, 0, 0, 2, 0, 58362, 1, 0, 0, 0, '', 'Fire credit requires the carried extinguisher'),
    (22, 3, 42940, 0, 0, 1, 0, 80199, 0, 0, 0, 0, '', 'Fire credit requires the periodic extinguisher effect'),
    (19, 0, 26391, 0, 1, 8, 0, 28817, 0, 0, 0, 0, '', 'Extinguishing Hope follows The Rear is Clear'),
    (19, 0, 26391, 0, 2, 8, 0, 28818, 0, 0, 0, 0, '', 'Extinguishing Hope follows The Rear is Clear'),
    (19, 0, 26391, 0, 3, 8, 0, 28819, 0, 0, 0, 0, '', 'Extinguishing Hope follows The Rear is Clear'),
    (19, 0, 26391, 0, 4, 8, 0, 28820, 0, 0, 0, 0, '', 'Extinguishing Hope follows The Rear is Clear'),
    (19, 0, 26391, 0, 5, 8, 0, 28821, 0, 0, 0, 0, '', 'Extinguishing Hope follows The Rear is Clear'),
    (19, 0, 26391, 0, 6, 8, 0, 28822, 0, 0, 0, 0, '', 'Extinguishing Hope follows The Rear is Clear'),
    (19, 0, 26391, 0, 7, 8, 0, 28823, 0, 0, 0, 0, '', 'Extinguishing Hope follows The Rear is Clear'),
    (19, 0, 26391, 0, 8, 8, 0, 29083, 0, 0, 0, 0, '', 'Extinguishing Hope follows The Rear is Clear'),
    (19, 0, 26391, 0, 9, 8, 0, 31145, 0, 0, 0, 0, '', 'Extinguishing Hope follows The Rear is Clear');

-- Finish the same acknowledged hit after granting credit, including the eighth fire.
UPDATE `smart_scripts` SET `link` = 4
WHERE `entryorguid` = 42940 AND `source_type` = 0 AND `id` = 3 AND `event_type` = 61 AND `action_type` = 22 AND `action_param1` = 2;

UPDATE `smart_scripts` SET `event_type` = 61, `event_param1` = 0, `event_param3` = 0, `event_param4` = 0,
    `action_param1` = 0, `comment` = 'Northshire Vineyards Fire Trigger - Extinguish after the credited hit'
WHERE `entryorguid` = 42940 AND `source_type` = 0 AND `id` = 4 AND `event_type` = 8 AND `event_param1` = 80208 AND `action_type` = 37;
