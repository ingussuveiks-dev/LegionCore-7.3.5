UPDATE spell_scene SET ScriptName='scene_silver_hand_collapse'
WHERE MiscValue=1268 AND SceneScriptPackageID=1619 AND PlaybackFlags=27 AND CustomDuration=0
  AND (ScriptName IS NULL OR ScriptName='')
  AND NOT EXISTS (SELECT 1 FROM spell_scene_event WHERE MiscValue=1268)
  AND EXISTS (SELECT 1 FROM spell_target_position WHERE id=211867 AND target_map=1539
      AND ABS(target_position_x-2136.63)<0.01 AND ABS(target_position_y-2399.31)<0.01
      AND ABS(target_position_z-118.81)<0.01 AND target_orientation=0);
