START TRANSACTION;

SET @warswords_weapon_phase := (SELECT COUNT(*) FROM gameobject WHERE id=248832)=1
AND EXISTS (SELECT 1 FROM gameobject WHERE guid=134150 AND id=248832 AND map=1511 AND zoneId=7541 AND areaId=7830
    AND PhaseId='7001 7003' AND phaseMask=1 AND spawnMask=4096
    AND ABS(position_x-3515.24)<0.01 AND ABS(position_y-1800.51)<0.01 AND ABS(position_z-0.538696)<0.001)
AND EXISTS (SELECT 1 FROM gameobject_template WHERE entry=248832 AND type=3 AND displayId=9806 AND flags=2113536
    AND ScriptName='go_art_warswords' AND AIName='')
AND EXISTS (SELECT 1 FROM phase_definitions WHERE zoneId=7541 AND entry=51 AND phaseId='7001' AND phasemask=3 AND flags=16)
AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=51)=1
AND EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=51
    AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=44 AND ConditionTarget=0
    AND ConditionValue1=944 AND ConditionValue2=6 AND ConditionValue3=0 AND NegativeCondition=0 AND ErrorTextId=0 AND ScriptName='');

UPDATE gameobject SET PhaseId='7001'
WHERE guid=134150 AND id=248832 AND map=1511 AND PhaseId='7001 7003' AND @warswords_weapon_phase;

COMMIT;
