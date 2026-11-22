-- Use the six native broadcast texts for the personal Old Friends aircraft's seated pilot.
INSERT INTO `creature_text`
    (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`BroadcastTextID`,
     `MinTimer`,`MaxTimer`,`SpellID`,`comment`)
VALUES
    (38869,0,0,'',14,0,100,0,0,0,38943,0,0,0,'Old Friends - takeoff'),
    (38869,1,0,'',14,0,100,0,0,0,38958,0,0,0,'Old Friends - Oomlot Village'),
    (38869,2,0,'',14,0,100,0,0,0,38970,0,0,0,'Old Friends - Town-In-A-Box approach'),
    (38869,3,0,'',14,0,100,0,0,0,38971,0,0,0,'Old Friends - deserted town'),
    (38869,4,0,'',14,0,100,0,0,0,38973,0,0,0,'Old Friends - heading to the orcs'),
    (38869,5,0,'',14,0,100,0,0,0,38992,0,0,0,'Old Friends - final approach')
ON DUPLICATE KEY UPDATE
    `Type`=VALUES(`Type`), `Language`=VALUES(`Language`), `Probability`=VALUES(`Probability`),
    `Emote`=VALUES(`Emote`), `Duration`=VALUES(`Duration`), `Sound`=VALUES(`Sound`),
    `BroadcastTextID`=VALUES(`BroadcastTextID`), `MinTimer`=VALUES(`MinTimer`), `MaxTimer`=VALUES(`MaxTimer`),
    `SpellID`=VALUES(`SpellID`), `comment`=VALUES(`comment`);
