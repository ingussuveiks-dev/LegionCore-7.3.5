START TRANSACTION;

SET @talonclaw_ambush_scene_admission :=
    EXISTS (SELECT 1 FROM eventobject_template WHERE entry=108 AND ScriptName='SmartEventObject' AND radius=24 AND SpellID=0 AND WorldSafeLocID=0 AND Flags=0)
    AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=108 AND source_type=13)=3
    AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=108 AND source_type=13 AND event_phase_mask=0 AND event_chance=100
        AND ((id=0 AND link=1 AND event_type=89 AND event_flags=0 AND action_type=85 AND action_param1=198883 AND target_type=7)
          OR (id=1 AND link=0 AND event_type=61 AND action_type=45 AND action_param1=5 AND action_param2=5 AND target_type=19 AND target_param1=100235)
          OR (id=2 AND link=0 AND event_type=89 AND event_flags=1 AND action_type=85 AND action_param1=197615 AND target_type=7)))=3
    AND EXISTS (SELECT 1 FROM spell_scene WHERE MiscValue=1145 AND SceneScriptPackageID=1517 AND PlaybackFlags=25 AND CustomDuration=0 AND COALESCE(ScriptName,'')='')
    AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=108 AND SourceId=10)=2
    AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=108 AND SourceId=13)
    AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=108 AND SourceId=10
        AND ElseGroup=0 AND ConditionTypeOrReference=44 AND ConditionTarget=0 AND ConditionValue1=973 AND ConditionValue3=0
        AND NegativeCondition=0 AND ErrorTextId=0 AND ScriptName=''
        AND ((SourceGroup=1 AND ConditionValue2=10) OR (SourceGroup=3 AND ConditionValue2=11)))=2;

UPDATE conditions SET SourceId=13
WHERE SourceTypeOrReferenceId=22 AND SourceEntry=108 AND SourceId=10 AND @talonclaw_ambush_scene_admission;

COMMIT;
