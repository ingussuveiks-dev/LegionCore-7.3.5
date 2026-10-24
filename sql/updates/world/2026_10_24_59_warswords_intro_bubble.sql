START TRANSACTION;

SET @warswords_intro_bubble := (SELECT COUNT(*) FROM creature WHERE id=98602)=1
AND EXISTS (SELECT 1 FROM creature WHERE guid=361283 AND id=98602 AND map=1511 AND spawnMask=4096
    AND PhaseId='7003' AND phaseMask=1 AND modelid=0 AND MovementType=0
    AND ABS(position_x-3515.15)<0.01 AND ABS(position_y-1800.61)<0.01 AND ABS(position_z-0.630465)<0.01)
AND EXISTS (SELECT 1 FROM creature_template WHERE entry=98602 AND ScriptName='npc_vigfus_bladewind_last' AND AIName='')
AND NOT EXISTS (SELECT 1 FROM creature_template_addon WHERE entry=98602);

INSERT INTO creature_addon (guid,path_id,mount,bytes1,bytes2,emote,auras)
SELECT 361283,0,0,0,1,0,'205424'
WHERE @warswords_intro_bubble AND NOT EXISTS (SELECT 1 FROM creature_addon WHERE guid=361283);

COMMIT;
