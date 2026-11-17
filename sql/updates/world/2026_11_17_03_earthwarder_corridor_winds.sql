-- Activate only with spell_earthwarder_spirit_winds in the matching executable.
-- Keep shared 4727/190842+190845 and all other scenarios unchanged.
SET @ew_wind_ok := (
    (SELECT COUNT(*) FROM `areatrigger_template` WHERE `entry`=4727 AND `customEntry`=9456
        AND `spellId` IN (190842,190845) AND `VisualID`=49393
        AND `Radius`=35 AND `Height`=12.5 AND `hasAreaTriggerBox`=1) = 2
    AND (SELECT COUNT(*) FROM `creature` WHERE `id`=96740 AND `map`=1495 AND `PhaseId`=6999
        AND `guid` IN (361407,361409)) = 2
    AND NOT EXISTS (SELECT 1 FROM `areatrigger_template` WHERE `customEntry`=190842
        AND (`entry`<>0 OR `spellId`<>190842 OR `comment`<>'Earthwarder owned corridor winds'))
    AND NOT EXISTS (SELECT 1 FROM `areatrigger_data` WHERE `customEntry`=190842
        AND (`entry`<>0 OR `spellId`<>190842))
    AND NOT EXISTS (SELECT 1 FROM `areatrigger_actions` WHERE `customEntry`=190842
        AND (`entry`<>0 OR `id`<>102 OR `comment`<>'Earthwarder owned wind handoff retry'))
    AND NOT EXISTS (SELECT 1 FROM `areatrigger_scripts` WHERE `entry`=190842)
    AND NOT EXISTS (SELECT 1 FROM `spell_script_names` WHERE `spell_id`=190842
        AND `ScriptName`<>'spell_earthwarder_spirit_winds')
);
SET @ew_wind_guard := IF(@ew_wind_ok, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Earthwarder corridor wind context differs''');
PREPARE ew_wind_check FROM @ew_wind_guard;
EXECUTE ew_wind_check;
DEALLOCATE PREPARE ew_wind_check;

INSERT INTO `areatrigger_template`
    (`entry`,`spellId`,`customEntry`,`VisualID`,`Radius`,`RadiusTarget`,`Height`,`HeightTarget`,
     `Float4`,`Float5`,`hasAreaTriggerBox`,`windX`,`windY`,`windZ`,`windSpeed`,`windType`,`comment`)
VALUES (0,190842,190842,49393,35,35,6.25,6.25,5,5,1,-2.54187,-14.7831,0,15,0,'Earthwarder owned corridor winds')
ON DUPLICATE KEY UPDATE `Radius`=35,`RadiusTarget`=35,`Height`=6.25,`HeightTarget`=6.25,
    `Float4`=5,`Float5`=5,`windX`=-2.54187,`windY`=-14.7831,`windZ`=0,`windSpeed`=15,`windType`=0;

INSERT INTO `areatrigger_data` (`entry`,`spellId`,`customEntry`,`updateDelay`)
VALUES (0,190842,190842,200)
ON DUPLICATE KEY UPDATE `updateDelay`=200;

-- Native auto actions100/101 own enter/leave/removal. Reapply only when the
-- previous strip's force is gone (or a native force immunity has ended).
INSERT INTO `areatrigger_actions`
    (`entry`,`customEntry`,`id`,`moment`,`actionType`,`targetFlags`,`spellId`,`comment`)
VALUES (0,190842,102,4,6,8,190842,'Earthwarder owned wind handoff retry')
ON DUPLICATE KEY UPDATE `moment`=4,`actionType`=6,`targetFlags`=8,`spellId`=190842;

INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`)
SELECT 190842,'spell_earthwarder_spirit_winds' FROM DUAL
WHERE NOT EXISTS (SELECT 1 FROM `spell_script_names`
    WHERE `spell_id`=190842 AND `ScriptName`='spell_earthwarder_spirit_winds');

SET @ew_wind_ok := NULL;
SET @ew_wind_guard := NULL;
