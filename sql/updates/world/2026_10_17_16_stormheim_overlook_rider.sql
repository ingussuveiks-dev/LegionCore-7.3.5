-- Masters of Disguise ender: two identical riders occupy the same position.
-- Keep DB340420. Guard the duplicate341178 against customization/dependencies;
-- compare every spawn field so local phase, pose or movement changes survive.
DELETE duplicate FROM creature duplicate JOIN creature original ON original.guid=340420
WHERE duplicate.guid=341178 AND original.id=97250 AND original.map=1220
  AND duplicate.id=original.id AND duplicate.map=original.map
  AND duplicate.zoneId=original.zoneId AND duplicate.areaId=original.areaId
  AND duplicate.spawnMask=original.spawnMask AND duplicate.phaseMask=original.phaseMask
  AND duplicate.PhaseId <=> original.PhaseId
  AND duplicate.modelid=original.modelid AND duplicate.equipment_id=original.equipment_id
  AND duplicate.position_x=original.position_x AND duplicate.position_y=original.position_y
  AND duplicate.position_z=original.position_z AND duplicate.orientation=original.orientation
  AND duplicate.spawntimesecs=original.spawntimesecs AND duplicate.spawndist=original.spawndist
  AND duplicate.currentwaypoint=original.currentwaypoint AND duplicate.curhealth=original.curhealth
  AND duplicate.curmana=original.curmana AND duplicate.MovementType=original.MovementType
  AND duplicate.npcflag=original.npcflag AND duplicate.npcflag2=original.npcflag2
  AND duplicate.unit_flags=original.unit_flags AND duplicate.dynamicflags=original.dynamicflags
  AND duplicate.AiID=original.AiID AND duplicate.MovementID=original.MovementID
  AND duplicate.MeleeID=original.MeleeID AND duplicate.isActive=original.isActive
  AND duplicate.skipClone=original.skipClone AND duplicate.personal_size=original.personal_size
  AND duplicate.isTeemingSpawn=original.isTeemingSpawn AND duplicate.unit_flags3=original.unit_flags3
  AND NOT EXISTS (SELECT 1 FROM creature_addon a WHERE a.guid IN(original.guid,duplicate.guid))
  AND NOT EXISTS (SELECT 1 FROM smart_scripts s WHERE (s.source_type=0 AND s.entryorguid=-CAST(duplicate.guid AS SIGNED)) OR (s.target_type=10 AND s.target_param1=duplicate.guid))
  AND NOT EXISTS (SELECT 1 FROM conditions c WHERE c.SourceEntry=-CAST(duplicate.guid AS SIGNED) OR c.ConditionValue3=duplicate.guid)
  AND NOT EXISTS (SELECT 1 FROM pool_creature p WHERE p.guid IN(original.guid,duplicate.guid))
  AND NOT EXISTS (SELECT 1 FROM game_event_creature e WHERE e.guid IN(original.guid,duplicate.guid))
  AND NOT EXISTS (SELECT 1 FROM linked_respawn r WHERE r.guid IN(original.guid,duplicate.guid) OR r.linkedGuid IN(original.guid,duplicate.guid))
  AND NOT EXISTS (SELECT 1 FROM creature_formations f WHERE f.leaderGUID IN(original.guid,duplicate.guid) OR f.memberGUID IN(original.guid,duplicate.guid));
