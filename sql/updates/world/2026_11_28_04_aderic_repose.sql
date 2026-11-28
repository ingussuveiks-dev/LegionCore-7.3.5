-- Laid to Rest: five Unearthed Mementos from the shared soil or Disturbed Spirits.
UPDATE `gameobject_template` SET `Data8`=24602 WHERE `entry`=201871 AND `type`=3;
UPDATE `gameobject_loot_template` SET `QuestRequired`=1 WHERE `Entry`=201871 AND `Item`=49921 AND `LootMode`=1;
UPDATE `creature_loot_template` SET `QuestRequired`=1, `LootMode`=1 WHERE `Entry`=37884 AND `Item`=49921;
