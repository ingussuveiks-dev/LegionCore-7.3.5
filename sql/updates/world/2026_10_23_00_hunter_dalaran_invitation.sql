-- The native area spell summons personal Snowfeather, the giver of 40384.
-- quest_end_status is an allowed-status mask: 74 excludes QUEST_STATUS_NONE
-- and instead permits active/completed/rewarded characters. Offer only a new
-- or abandoned Needs of the Hunters; preserve custom rows and other classes.
UPDATE `spell_area` SET `quest_end_status`=1
WHERE `spell`=196908 AND `area`=7502 AND `quest_start`=0
  AND `quest_end`=40384 AND `classmask`=4 AND `autocast`=1
  AND `quest_end_status`=74;
