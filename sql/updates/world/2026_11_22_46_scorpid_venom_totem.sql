-- Keep both authored combat spell rows. Replace the proximity-only credit with the native Envenom hit script.
UPDATE `creature_template` SET `ScriptName`='npc_clattering_scorpid_venom'
WHERE `entry`=3125 AND `AIName`='SmartAI' AND `ScriptName` IN ('','npc_clattering_scorpid_venom');

DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId`=22 AND `SourceEntry`=3125 AND `SourceGroup`=3 AND `SourceId`=0
  AND `ElseGroup`=0 AND `ConditionTypeOrReference`=29 AND `ConditionTarget`=0
  AND `ConditionValue1`=39236 AND `ConditionValue2`=15 AND `NegativeCondition`=0;
DELETE FROM `smart_scripts`
WHERE `entryorguid`=3125 AND `source_type`=0 AND `id`=2 AND `event_type`=31
  AND `event_param1`=73672 AND `action_type`=33 AND `action_param1`=39236;

INSERT IGNORE INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
  (73672,'spell_clattering_scorpid_envenom'),
  (73673,'spell_poison_extraction_totem');
