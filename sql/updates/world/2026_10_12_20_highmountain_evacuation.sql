-- Evacuate Snowmane: personal departure scenes and guarded, distinct credits.
UPDATE `creature_template` SET `AIName`='', `ScriptName`='npc_highmountain_evacuation'
WHERE `entry` IN (106115,106153,106191) AND `AIName` IN ('SmartAI','')
  AND `ScriptName` IN ('','npc_highmountain_evacuation');

-- Remove only the obsolete gossip->instant credit/text action lists.
DELETE FROM `smart_scripts` WHERE `source_type`=0 AND `id`=0 AND `event_type`=62
  AND `action_type`=80 AND ((`entryorguid`=106115 AND `action_param1`=10611500)
    OR (`entryorguid`=106153 AND `action_param1`=10615300)
    OR (`entryorguid`=106191 AND `action_param1`=10619100));
DELETE FROM `smart_scripts` WHERE `source_type`=9 AND `entryorguid` IN (10611500,10615300,10619100)
  AND ((`id`=0 AND `action_type`=33 AND `action_param1` IN (106115,106153,106191))
    OR (`id`=1 AND `action_type`=1 AND `action_param1`=0));

UPDATE `gossip_menu_option` SET `OptionText`='There is no time left. Run!'
WHERE `MenuID` IN (19560,19561,19565) AND `OptionID`=0 AND `OptionBroadcastTextID`=109076;

-- 7.3.5 build 26124 hotfix broadcast texts; Tarvim previously had a corrupt fallback and no broadcast ID.
UPDATE `creature_text` SET `Text`='I am not leaving without my Kaya!', `BroadcastTextID`=109069
WHERE `CreatureID`=106115 AND `GroupID`=0 AND `ID`=0;
UPDATE `creature_text` SET `Text`='So be it. Come my wounded friends, let us see if I might have the strength that is needed to carry you to safety.', `BroadcastTextID`=109086
WHERE `CreatureID`=106153 AND `GroupID`=0 AND `ID`=0;
UPDATE `creature_text` SET `Text`='Fah! This is OUR mountain. Let the drogbar savages come. We have the power of the spirits, of this mountain. All they have are sticks and stones and anger.', `BroadcastTextID`=109107
WHERE `CreatureID`=106191 AND `GroupID`=0 AND `ID`=0;

-- Hide each option after its own objective, including while the other two remain.
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=15 AND `SourceGroup` IN (19560,19561,19565)
  AND `SourceEntry`=0 AND `SourceId`=0 AND `ElseGroup`=0 AND `ConditionValue1`=42088
  AND `ConditionTypeOrReference` IN (9,41);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,
  `ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,
  `NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(15,19560,0,0,0,9,0,42088,0,0,0,0,'','Evacuate Snowmane: active quest'),
(15,19560,0,0,0,41,0,42088,106115,1,1,0,'','Evacuate Snowmane: Red not yet evacuated'),
(15,19561,0,0,0,9,0,42088,0,0,0,0,'','Evacuate Snowmane: active quest'),
(15,19561,0,0,0,41,0,42088,106153,1,1,0,'','Evacuate Snowmane: Tarvim not yet evacuated'),
(15,19565,0,0,0,9,0,42088,0,0,0,0,'','Evacuate Snowmane: active quest'),
(15,19565,0,0,0,41,0,42088,106191,1,1,0,'','Evacuate Snowmane: Nefu not yet warned');
