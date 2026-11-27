-- Put the visible flame emitters at the crest of the stair, above the mesh.
UPDATE `gameobject`
SET `position_z` = 44.10
WHERE `guid` IN (3101136, 3101137, 3101138) AND `id` = 246866
  AND `map` = 1494 AND `PhaseId` = '5015';
