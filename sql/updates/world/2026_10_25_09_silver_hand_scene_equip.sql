UPDATE spell_scene SET ScriptName='scene_silver_hand_pickup'
WHERE MiscValue=1267 AND SceneScriptPackageID=1618 AND PlaybackFlags=21 AND CustomDuration=0
  AND (ScriptName IS NULL OR ScriptName='')
  AND NOT EXISTS (SELECT 1 FROM spell_scene_event WHERE MiscValue=1267);
