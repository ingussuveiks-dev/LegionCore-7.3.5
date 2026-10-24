START TRANSACTION;

SET @talonclaw_apata_voice :=
    EXISTS (SELECT 1 FROM creature_template WHERE entry=100235 AND AIName='SmartAI' AND ScriptName='')
    AND EXISTS (SELECT 1 FROM creature_template_wdb WHERE Entry=100235 AND DisplayID1=66135 AND DisplayID2=0 AND DisplayID3=0 AND DisplayID4=0)
    AND EXISTS (SELECT 1 FROM creature_model_info WHERE DisplayID=66135 AND DisplayID_Other_Gender=0)
    AND NOT EXISTS (SELECT 1 FROM creature WHERE id=100235 AND modelid NOT IN(0,66135))
    AND (SELECT COUNT(*) FROM creature_text WHERE CreatureID=100235 AND GroupID BETWEEN 11 AND 14)=4
    AND (SELECT COUNT(*) FROM creature_text WHERE CreatureID=100235 AND ID=0 AND Language=0 AND Probability=100
        AND Emote=0 AND Duration=0 AND Sound=0 AND MinTimer=0 AND MaxTimer=0 AND SpellID=0
        AND ((GroupID=11 AND Type=14 AND BroadcastTextID IN(0,102113) AND SHA2(Text,256)='90ae1a3896d10531e0c871b371e51de79e012096f1acbc6c440fda7b54dcb9f9')
          OR (GroupID=12 AND Type=12 AND BroadcastTextID IN(0,102114) AND SHA2(Text,256)='6c45c590282ee7c596313ab2cf694a8ee09458a120880a82c8172f9b3df7b685')
          OR (GroupID=13 AND Type=12 AND BroadcastTextID IN(0,103426) AND SHA2(Text,256)='c04504f3892db2c7c1c3ecdbef4b067b5a578f4e4cf66d8d48218c47585ec6ec')
          OR (GroupID=14 AND Type=12 AND BroadcastTextID IN(0,103430) AND SHA2(Text,256)='d43d49f32d4fcaeb732b99cfae8b6e44cce64d2d612d7e8f61af26fc71f2fcf6')))=4
    AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=100235 AND source_type=0 AND action_type=1
        AND ((id=15 AND action_param1=11 AND action_param2=3000)
          OR (id=16 AND action_param1=12 AND action_param2=6000)
          OR (id=18 AND action_param1=13 AND action_param2=8000)
          OR (id=19 AND action_param1=14 AND action_param2=7000)))=4;

UPDATE creature_text SET BroadcastTextID=CASE GroupID
    WHEN 11 THEN 102113 WHEN 12 THEN 102114 WHEN 13 THEN 103426 WHEN 14 THEN 103430 END
WHERE CreatureID=100235 AND GroupID BETWEEN 11 AND 14 AND ID=0 AND @talonclaw_apata_voice;

COMMIT;
