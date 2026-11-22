-- Slaver's Keys are native use-item keys for the Ball and Chain locks.
INSERT IGNORE INTO `creature_loot_template`
(`Entry`,`Item`,`Reference`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`)
VALUES (37701,49881,0,70.4286,1,0,0,1,1);

INSERT IGNORE INTO `pickpocketing_loot_template`
(`Entry`,`Item`,`Reference`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`)
VALUES (37701,49881,0,70.4286,1,1,0,1,1);
