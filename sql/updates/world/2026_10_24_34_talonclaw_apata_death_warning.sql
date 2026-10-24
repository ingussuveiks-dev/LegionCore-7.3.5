START TRANSACTION;

SET @talonclaw_death_warning :=
    EXISTS (SELECT 1 FROM creature_template WHERE entry=100235 AND AIName='SmartAI' AND ScriptName='')
    AND EXISTS (SELECT 1 FROM creature_template_wdb WHERE Entry=100235 AND DisplayID1=66135 AND DisplayID2=0 AND DisplayID3=0 AND DisplayID4=0)
    AND NOT EXISTS (SELECT 1 FROM creature WHERE id=100235 AND modelid NOT IN(0,66135))
    AND (SELECT COUNT(*) FROM creature_text WHERE CreatureID=100235 AND GroupID=20)=1
    AND EXISTS (SELECT 1 FROM creature_text WHERE CreatureID=100235 AND GroupID=20 AND ID=0 AND Type=14 AND Language=0 AND Probability=100
        AND Emote=0 AND Duration=0 AND Sound=0 AND MinTimer=0 AND MaxTimer=0 AND SpellID=0 AND BroadcastTextID IN(0,102115)
        AND SHA2(Text,256)='0d8097ec7410b8d5ec59b827f1ba3034ba12e52eea24e455216ace8543de24c5')
    AND EXISTS (SELECT 1 FROM creature_text WHERE CreatureID=100235 AND GroupID=21 AND ID=0 AND BroadcastTextID=102116)
    AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=100235 AND source_type=0 AND id IN(21,22))=2
    AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=100235 AND source_type=0 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
        AND ((id=21 AND link=22 AND event_type=38 AND event_param1=6 AND event_param2=6 AND action_type=1 AND action_param1 IN(20,21) AND action_param2=0 AND target_type=1)
          OR (id=22 AND link=0 AND event_type=61 AND action_type=37 AND target_type=1)))=2
    AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100235 AND SourceId=0 AND SourceGroup IN(22,23))
    AND EXISTS (SELECT 1 FROM spell_scene WHERE MiscValue=1145 AND SceneScriptPackageID=1517 AND PlaybackFlags=25 AND CustomDuration=0 AND COALESCE(ScriptName,'')='');

UPDATE creature_text SET BroadcastTextID=102115
WHERE CreatureID=100235 AND GroupID=20 AND ID=0 AND @talonclaw_death_warning;

UPDATE smart_scripts SET action_param1=20
WHERE entryorguid=100235 AND source_type=0 AND id=21 AND @talonclaw_death_warning;

COMMIT;
