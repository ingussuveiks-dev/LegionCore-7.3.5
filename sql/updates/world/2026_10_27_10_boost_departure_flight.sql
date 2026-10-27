-- CODE-COUPLED: stage outside updater until matching takeoff rearm is deployed.
-- Keep perched ground behavior; native airborne updates must retain Fly tier.
START TRANSACTION;
SET @boost_bird_safe :=
 (SELECT COUNT(*) FROM creature_template WHERE entry IN(107543,112565) AND ScriptName='npc_boost_exit')=2
 AND NOT EXISTS(SELECT 1 FROM creature_template_movement WHERE CreatureId IN(107543,112565) AND Flight IS NOT NULL AND Flight NOT IN(0,1));
SET @boost_bird_guard := IF(@boost_bird_safe,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Boost departure bird script/flight differs; review before promotion''');
PREPARE boost_bird_check FROM @boost_bird_guard;
EXECUTE boost_bird_check;
DEALLOCATE PREPARE boost_bird_check;
INSERT INTO creature_template_movement(CreatureId,Flight)
SELECT entry,1 FROM creature_template WHERE @boost_bird_safe AND entry IN(107543,112565) AND ScriptName='npc_boost_exit'
ON DUPLICATE KEY UPDATE Flight=VALUES(Flight);
COMMIT;
