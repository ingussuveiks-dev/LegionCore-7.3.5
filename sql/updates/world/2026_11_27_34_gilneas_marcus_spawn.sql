-- Marcus (37803) at his build 26365 Livery Outpost placement and phase 186.
-- Aura 69484 supplies both the Legion phase ID and its older mask.
START TRANSACTION;

SET @gilneas_marcus_safe :=
    EXISTS (SELECT 1 FROM `creature_template` WHERE `entry`=37803 AND (`npcflag` & 2)<>0)
    AND EXISTS (SELECT 1 FROM `creature_queststarter` WHERE `id`=37803 AND `quest`=24674)
    AND EXISTS (SELECT 1 FROM `creature_questender` WHERE `id`=37803 AND `quest`=24674)
    AND EXISTS (SELECT 1 FROM `creature` WHERE `guid`=148907 AND `id`=37802 AND `map`=654)
    AND EXISTS (SELECT 1 FROM `quest_objectives`
        WHERE `ID`=265172 AND `QuestID`=24674 AND `ObjectID`=37802 AND `Amount`=1 AND `Type`=0)
    AND NOT EXISTS (SELECT 1 FROM `creature` WHERE (`guid`=3101074 OR `id`=37803)
        AND NOT (`guid`=3101074 AND `id`=37803 AND `map`=654 AND `zoneId`=4714 AND `areaId`=4842
            AND `spawnMask`=1 AND `phaseMask`=131072 AND `PhaseId`='186' AND `equipment_id`=0
            AND ABS(`position_x`+1369.7)<0.001 AND ABS(`position_y`-1202.8)<0.001 AND ABS(`position_z`-35.6354)<0.001
            AND ABS(`orientation`-5.09315)<0.001 AND `MovementType`=0))
    AND NOT EXISTS (SELECT 1 FROM `spell_area` WHERE `spell`=69484 AND `area` IN(4714,4842)
        AND NOT (`area`=4842 AND `quest_start`=24677 AND `quest_end`=0 AND `aura_spell`=0
            AND `racemask`=2097152 AND `classmask`=0 AND `active_event`=0 AND `gender`=2
            AND `autocast`=1 AND `quest_start_status`=74 AND `quest_end_status`=0));

SET @gilneas_marcus_guard := IF(@gilneas_marcus_safe, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Gilneas Marcus: native quest, placement or phasing differs''');
PREPARE gilneas_marcus_check FROM @gilneas_marcus_guard;
EXECUTE gilneas_marcus_check;
DEALLOCATE PREPARE gilneas_marcus_check;

INSERT INTO `creature`
    (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,`PhaseId`,`equipment_id`,
     `position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`MovementType`)
SELECT 3101074,37803,654,4714,4842,1,131072,'186',0,-1369.7,1202.8,35.6354,5.09315,7200,0
WHERE @gilneas_marcus_safe AND NOT EXISTS (SELECT 1 FROM `creature` WHERE `guid`=3101074);

INSERT INTO `spell_area`
    (`spell`,`area`,`quest_start`,`quest_end`,`aura_spell`,`racemask`,`classmask`,`active_event`,
     `gender`,`autocast`,`quest_start_status`,`quest_end_status`)
SELECT 69484,4842,24677,0,0,2097152,0,0,2,1,74,0
WHERE @gilneas_marcus_safe AND NOT EXISTS (SELECT 1 FROM `spell_area` WHERE `spell`=69484 AND `area`=4842);

COMMIT;
