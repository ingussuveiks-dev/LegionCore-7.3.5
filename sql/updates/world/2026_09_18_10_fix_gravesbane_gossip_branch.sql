-- 19637 is the failure/reset gossip menu for this dialogue, not a POI entry.
UPDATE `gossip_menu_option`
SET `ActionMenuID` = 19637,
    `ActionPoiID` = 0
WHERE `MenuID` = 19634
  AND `OptionID` = 0
  AND `ActionMenuID` = 0
  AND `ActionPoiID` = 19637;
