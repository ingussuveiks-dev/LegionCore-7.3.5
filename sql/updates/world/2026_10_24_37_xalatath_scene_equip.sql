UPDATE spell_scene SET ScriptName='scene_xalatath_pickup'
WHERE MiscValue=1187 AND SceneScriptPackageID=1553 AND PlaybackFlags=21 AND CustomDuration=0
  AND (ScriptName IS NULL OR ScriptName='')
  AND NOT EXISTS (SELECT 1 FROM spell_scene_event WHERE MiscValue=1187);
