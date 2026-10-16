-- The questgiver pedestal already exists in phase3554. Credit its actual use,
-- retaining the native turn-in and follow-up quest instead of adding fake loot.
UPDATE gameobject_template SET ScriptName = 'go_halls_of_valor_quest_aegis'
WHERE entry = 251991 AND (ScriptName = '' OR ScriptName = 'go_halls_of_valor_quest_aegis');

-- Native 7.3.5 flags3: tracked on minimap plus the existing sequenced flag2.
UPDATE quest_objectives SET Flags = Flags | 1
WHERE ID = 285235 AND QuestID = 40072 AND Type = 0 AND ObjectID = 95676 AND Amount = 1;
