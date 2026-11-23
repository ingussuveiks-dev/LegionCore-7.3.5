START TRANSACTION;

-- Gossip options are sent verbatim and both variants of the token say "ready".
UPDATE `gossip_menu_option`
SET `OptionText` = 'I''m ready; go on the attack.'
WHERE `MenuID` = 20974 AND `OptionID` = 0 AND `OptionBroadcastTextID` = 0
  AND BINARY `OptionText` = 'I''m $gready:ready; go on the attack.';

COMMIT;
