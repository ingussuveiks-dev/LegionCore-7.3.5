-- These five options are pure menu navigation.  OptionNpc 4 is the vendor
-- action and makes the core reject ActionMenuID, so the submenu never opens.
UPDATE `gossip_menu_option`
SET `OptionNpc` = 0
WHERE (`MenuID` = 600022 AND `OptionID` IN (30, 31, 32)
       AND `ActionMenuID` IN (600122, 600123, 600124))
   OR (`MenuID` IN (600123, 600124) AND `OptionID` = 0
       AND `ActionMenuID` = 600022);
