START TRANSACTION;

SET @talonclaw_apata_additional_voice :=
    EXISTS (SELECT 1 FROM creature_template WHERE entry=100235 AND AIName='SmartAI' AND ScriptName='')
    AND EXISTS (SELECT 1 FROM creature_template_wdb WHERE Entry=100235 AND DisplayID1=66135 AND DisplayID2=0 AND DisplayID3=0 AND DisplayID4=0)
    AND EXISTS (SELECT 1 FROM creature_model_info WHERE DisplayID=66135 AND DisplayID_Other_Gender=0)
    AND NOT EXISTS (SELECT 1 FROM creature WHERE id=100235 AND modelid NOT IN(0,66135))
    AND (SELECT COUNT(*) FROM creature_text WHERE CreatureID=100235 AND GroupID IN(2,21))=2
    AND (SELECT COUNT(*) FROM creature_text WHERE CreatureID=100235 AND ID=0 AND Type=12 AND Language=0 AND Probability=100
        AND Emote=0 AND Duration=0 AND Sound=0 AND MinTimer=0 AND MaxTimer=0 AND SpellID=0
        AND ((GroupID=2 AND BroadcastTextID IN(65559,102002) AND SHA2(Text,256)='aa86760e85930bb8165a01a5b1f2b29b52d5d48f1e60a00a92564ef5a71ac738')
          OR (GroupID=21 AND BroadcastTextID IN(0,102116) AND SHA2(Text,256)='f2fddb52880839002b7d00daf6c85097d636710840f35c152802d37c95ecdc99')))=2
    AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=100235 AND source_type=0 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
        AND ((id=8 AND link=0 AND event_type=60 AND action_type=1 AND action_param1=2 AND action_param2=4000 AND target_type=1)
          OR (id=21 AND link=22 AND event_type=38 AND event_param1=6 AND event_param2=6 AND action_type=1 AND action_param1=21 AND action_param2=0 AND target_type=1)
          OR (id=22 AND link=0 AND event_type=61 AND action_type=37 AND target_type=1)))=3;

UPDATE creature_text SET BroadcastTextID=CASE GroupID WHEN 2 THEN 102002 WHEN 21 THEN 102116 END
WHERE CreatureID=100235 AND GroupID IN(2,21) AND ID=0 AND @talonclaw_apata_additional_voice;

COMMIT;
