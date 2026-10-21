-- All three shipped rows dispatch the same shipment service, not three types
-- of imps. Retain option 0 and remove only its two identical duplicates.
DELETE duplicate FROM gossip_menu_option duplicate
 JOIN gossip_menu_option original ON original.MenuID=duplicate.MenuID AND original.OptionID=0
 WHERE duplicate.MenuID=19918 AND duplicate.OptionID IN(1,2)
 AND original.OptionNpc=28 AND original.OptionBroadcastTextID=115147
 AND duplicate.OptionNpc=original.OptionNpc
 AND duplicate.OptionText <=> original.OptionText
 AND duplicate.OptionBroadcastTextID=original.OptionBroadcastTextID
 AND duplicate.ActionMenuID=original.ActionMenuID AND duplicate.ActionPoiID=original.ActionPoiID
 AND duplicate.BoxCoded=original.BoxCoded AND duplicate.BoxMoney=original.BoxMoney
 AND duplicate.BoxText <=> original.BoxText
 AND duplicate.BoxBroadcastTextID=original.BoxBroadcastTextID;
