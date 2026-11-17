-- Preserve the original airborne spawn poses of the seven Warrior crypt
-- Swirling Storms. CanFly permits flight but leaves gravity enabled; these
-- stationary emitters need DisableGravity, scoped to their original spawns.
SET @storm_hover_ok := (
 SELECT COUNT(*) = 7 FROM creature c
 JOIN creature_template t ON t.entry=c.id
 JOIN creature_template_movement m ON m.CreatureId=c.id
 LEFT JOIN creature_movement_override o ON o.SpawnId=c.guid
 WHERE c.guid IN(361392,361394,361403,361405,361406,361412,361413)
 AND c.id=96455 AND c.map=1495 AND c.PhaseId=6999
 AND t.AIName='SmartAI' AND t.ScriptName=''
 AND m.Ground=1 AND m.Flight=2 AND m.Rooted=0
 AND (o.SpawnId IS NULL OR (o.Ground=0 AND o.Flight=1))
);
SET @storm_hover_guard := IF(@storm_hover_ok=1,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Earthwarder storm hover: original spawn or movement context differs''');
PREPARE storm_hover_check FROM @storm_hover_guard;
EXECUTE storm_hover_check;
DEALLOCATE PREPARE storm_hover_check;
INSERT INTO creature_movement_override
 (SpawnId,Ground,Swim,Flight,Rooted,Random,InteractionPauseTimer)
SELECT c.guid,0,m.Swim,1,m.Rooted,m.Random,m.InteractionPauseTimer
FROM creature c JOIN creature_template_movement m ON m.CreatureId=c.id
WHERE @storm_hover_ok=1 AND c.guid IN(361392,361394,361403,361405,361406,361412,361413)
 AND c.id=96455 AND c.map=1495 AND c.PhaseId=6999
ON DUPLICATE KEY UPDATE Ground=0,Flight=1;
SET @storm_hover_guard := NULL;
SET @storm_hover_ok := NULL;
