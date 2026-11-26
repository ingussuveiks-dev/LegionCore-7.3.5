-- Use the paid healing handler for the named stable masters and their pet battle trainers.
DELETE FROM `smart_scripts`
WHERE `source_type` = 0 AND `target_type` = 7
    AND (`entryorguid`,`id`,`link`,`event_type`,`event_param1`,`event_param2`,`action_type`,`action_param1`) IN
    (
        (6749,0,1,62,9821,2,33,64320),(6749,1,0,61,0,0,85,133994),
        (9987,0,1,62,9821,2,85,133994),(9987,1,0,61,0,0,33,65180),
        (10050,0,1,62,9821,2,33,65184),(10050,1,0,61,0,0,85,133994),
        (10055,0,1,62,100550,1,33,65193),(10055,1,0,61,0,0,85,133994),
        (17485,0,1,62,9821,2,33,65179),(17485,1,0,61,0,0,85,133994),
        (47764,0,1,62,14991,1,33,65212),(47764,1,0,61,0,0,85,133994),
        (63014,0,1,62,66420,0,33,64320),(63014,1,0,61,0,0,85,133994),
        (63061,0,1,62,66420,0,33,65180),(63061,1,0,61,0,0,85,133994),
        (63067,0,1,62,66420,0,33,65184),(63067,1,0,61,0,0,85,133994),
        (63070,0,1,62,66420,0,33,65041),(63070,1,0,61,0,0,85,133994),
        (63073,0,0,62,66420,0,85,133994),
        (63075,0,1,62,66420,0,33,65020),(63075,1,0,61,0,0,85,133994),
        (63077,0,0,62,66420,0,85,133994),(63077,0,1,62,66420,0,33,65179),(63077,1,0,61,0,0,85,133994),
        (63080,0,1,62,66420,0,33,65198),(63080,1,0,61,0,0,85,133994),
        (63083,0,1,62,66420,0,33,65199),(63083,1,0,61,0,0,85,133994),
        (63086,0,1,62,66420,0,33,65200),(63086,1,0,61,0,0,85,133994),
        (63626,0,0,62,14991,1,85,133994)
    );

-- Shelby and Seriadne are stable masters with no assigned gossip menu.
UPDATE `creature_template` SET `gossip_menu_id` = 9821 WHERE `entry` IN (9980,10051) AND `gossip_menu_id` = 0;

-- Bezzil has his own vendor menu. Add the standard paid service alongside those goods.
INSERT IGNORE INTO `gossip_menu_option`
    (`MenuID`,`OptionID`,`OptionNpc`,`OptionText`,`OptionBroadcastTextID`,`ActionMenuID`,`ActionPoiID`,`BoxCoded`,`BoxMoney`,`BoxText`,`BoxBroadcastTextID`,`VerifiedBuild`)
SELECT 12106,2,`OptionNpc`,`OptionText`,`OptionBroadcastTextID`,`ActionMenuID`,`ActionPoiID`,`BoxCoded`,`BoxMoney`,`BoxText`,`BoxBroadcastTextID`,0
FROM `gossip_menu_option` WHERE `MenuID` = 9821 AND `OptionID` = 2 AND `OptionBroadcastTextID` = 64115 AND `BoxMoney` = 1000;

-- Restore the missing offers for the trainers that already receive these quests.
INSERT IGNORE INTO `creature_queststarter` (`id`,`quest`)
SELECT `id`,`quest` FROM `creature_questender`
WHERE (`id`,`quest`) IN ((63070,31552),(63070,31553),(63070,31826),
    (63075,31548),(63075,31549),(63083,31582),(63083,31583),(63086,31586));
