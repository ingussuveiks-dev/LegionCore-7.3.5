-- Masters of Disguise: attach Shwayder's existing sniffed patrol. A spawn
-- addon overrides the template addon, so retain the template's disguise auras
-- and pose fields when creating it. Preserve an explicitly assigned path.
INSERT INTO creature_addon (guid,path_id,mount,bytes1,bytes2,emote,auras)
SELECT c.guid,288637,a.mount,a.bytes1,a.bytes2,a.emote,a.auras
FROM creature c JOIN creature_template_addon a ON a.entry=c.id
WHERE c.guid=288637 AND c.id=91460 AND c.map=1220 AND c.MovementType=2
  AND a.path_id=0
  AND EXISTS (SELECT 1 FROM waypoint_data WHERE id=288637)
  AND NOT EXISTS (SELECT 1 FROM creature_addon x WHERE x.guid=c.guid);

UPDATE creature_addon a JOIN creature c ON c.guid=a.guid
SET a.path_id=288637
WHERE c.guid=288637 AND c.id=91460 AND c.map=1220 AND c.MovementType=2
  AND a.path_id=0
  AND EXISTS (SELECT 1 FROM waypoint_data WHERE id=288637);
