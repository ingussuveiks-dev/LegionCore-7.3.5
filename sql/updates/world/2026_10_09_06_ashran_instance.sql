-- Legion 7.3.5: map 1191 / difficulty 25, zone 8485.
-- The controller belongs to the InstanceMap, never outdoorpvp_template.
INSERT INTO instance_template (map, parent, script, allowMount, bonusChance)
SELECT 1191, 1116, 'instance_ashran', 1, 0
WHERE NOT EXISTS (SELECT 1 FROM instance_template WHERE map = 1191);
UPDATE instance_template SET script = 'instance_ashran', allowMount = 1
WHERE map = 1191 AND script IN ('', 'instance_ashran');

-- Keep the original faction objectives and reward tables. Remove only the
-- impossible self prerequisites and erroneous cross-faction Slay continuation.
UPDATE quest_template_addon SET PrevQuestID = 0
WHERE ID IN (38923,39090) AND PrevQuestID = ID;
UPDATE quest_template SET RewardNextQuest = 0
WHERE ID IN (38923,39090) AND RewardNextQuest = ID;
UPDATE quest_template_addon SET NextQuestID = 0
WHERE ID = 39096 AND NextQuestID = 38924;
UPDATE quest_template SET RewardNextQuest = 0
WHERE ID = 39096 AND RewardNextQuest = 38924;
UPDATE quest_template SET Flags = Flags | 32768
WHERE ID IN (38923,38925,39090,39096);
UPDATE quest_template_addon SET SpecialFlags = SpecialFlags | 1
WHERE ID IN (38923,38925,39090,39096);
UPDATE quest_template SET FlagsEx = FlagsEx | 2097152 | 16777216
WHERE ID IN (38923,38925);
UPDATE quest_template SET LogTitle = 'Ashran Dominance'
WHERE ID = 38923 AND (LogTitle IS NULL OR LogTitle IN ('','NULL'));
UPDATE quest_template SET LogTitle = 'Slay Them All!'
WHERE ID = 39090 AND (LogTitle IS NULL OR LogTitle IN ('','NULL'));
