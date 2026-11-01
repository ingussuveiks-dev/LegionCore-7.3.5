-- Code-coupled: promote matching Release before exposing this updater input.
-- Original251591 is the door at the Scroll Keeper's Sanctuary, not a scroll.
-- Consume inadmissible Use before native no-close door state can latch.
-- Keep native SmartAI205/52452/49141, Lock2570, display11896/visual58002 and
-- the actual door/open/reset lifecycle. Only this ScriptName changes.
START TRANSACTION;
SET @fuzan_sanctuary_door_safe :=
 (SELECT COUNT(*) FROM `gameobject_template` WHERE entry=251591)=1
 AND EXISTS(SELECT 1 FROM `gameobject_template` WHERE `entry` <=> 251591 AND `type` <=> 0 AND `displayId` <=> 11896 AND `name` <=> CONVERT(0x446f6f7220617420746865205363726f6c6c204b656570657227732053616e637475617279 USING utf8mb4) AND `IconName` <=> CONVERT(0x7175657374696e746572616374 USING utf8mb4) AND `castBarCaption` <=> '' AND `unk1` <=> '' AND `faction` <=> 0 AND `flags` <=> 0 AND ABS(`size`-(0.95))<1e-06 AND `questItem1` <=> 0 AND `questItem2` <=> 0 AND `questItem3` <=> 0 AND `questItem4` <=> 0 AND `questItem5` <=> 0 AND `questItem6` <=> 0 AND `Data0` <=> 0 AND `Data1` <=> 2570 AND `Data2` <=> 0 AND `Data3` <=> 0 AND `Data4` <=> 0 AND `Data5` <=> 0 AND `Data6` <=> 0 AND `Data7` <=> 0 AND `Data8` <=> 0 AND `Data9` <=> 1 AND `Data10` <=> 0 AND `Data11` <=> 0 AND `Data12` <=> 0 AND `Data13` <=> 0 AND `Data14` <=> 0 AND `Data15` <=> 0 AND `Data16` <=> 0 AND `Data17` <=> 0 AND `Data18` <=> 0 AND `Data19` <=> 0 AND `Data20` <=> 0 AND `Data21` <=> 0 AND `Data22` <=> 0 AND `Data23` <=> 0 AND `Data24` <=> 0 AND `Data25` <=> 0 AND `Data26` <=> 0 AND `Data27` <=> 0 AND `Data28` <=> 0 AND `Data29` <=> 0 AND `Data30` <=> 0 AND `Data31` <=> 0 AND `Data32` <=> 0 AND `unkInt32` <=> 0 AND `AIName` <=> CONVERT(0x536d61727447616d654f626a6563744149 USING utf8mb4) AND `ScriptName` IN('','go_fuzan_temple_sanctuary_door') AND `WorldEffectID` <=> 0 AND `SpellVisualID` <=> 58002 AND `SpellStateVisualID` <=> 0 AND `SpellStateAnimID` <=> 0 AND `SpellStateAnimKitID` <=> 0 AND `StateWorldEffectID` <=> 0 AND `MaxVisible` <=> 0 AND `IgnoreDynLos` <=> 0 AND `MinGold` <=> 0 AND `MaxGold` <=> 0)
 AND (SELECT COUNT(*) FROM `smart_scripts` WHERE entryorguid=251591 AND source_type=1)=1
 AND EXISTS(SELECT 1 FROM `smart_scripts` WHERE `entryorguid` <=> 251591 AND `source_type` <=> 1 AND `id` <=> 0 AND `link` <=> 0 AND `Difficulties` <=> '' AND `event_type` <=> 64 AND `event_phase_mask` <=> 0 AND `event_chance` <=> 100 AND `event_flags` <=> 1 AND `event_param1` <=> 0 AND `event_param2` <=> 0 AND `event_param3` <=> 0 AND `event_param4` <=> 0 AND `event_param5` <=> 0 AND `action_type` <=> 205 AND `action_param1` <=> 92 AND `action_param2` <=> 52452 AND `action_param3` <=> 0 AND `action_param4` <=> 0 AND `action_param5` <=> 0 AND `action_param6` <=> 0 AND `target_type` <=> 7 AND `target_param1` <=> 0 AND `target_param2` <=> 0 AND `target_param3` <=> 0 AND `target_param4` <=> 0 AND `target_x` <=> 0 AND `target_y` <=> 0 AND `target_z` <=> 0 AND `target_o` <=> 0 AND `comment` <=> CONVERT(0x476f737369702048656c6c6f202d20435343 USING utf8mb4))
 AND (SELECT COUNT(*) FROM `conditions` WHERE SourceTypeOrReferenceId=22 AND SourceEntry=251591)=0
 AND (SELECT COUNT(*) FROM `gameobject` WHERE id=251591)=1
 AND EXISTS(SELECT 1 FROM `gameobject` WHERE `guid` <=> 138378 AND `id` <=> 251591 AND `map` <=> 1625 AND `phaseMask` <=> 1 AND `phaseId` <=> CONVERT(0x36393035 USING utf8mb4) AND ABS(`position_x`-(917.288))<0.0005 AND ABS(`position_y`-(-2496.43))<0.0005 AND ABS(`position_z`-(180.524))<0.0005 AND ABS(`orientation`-(2.83113))<0.0005 AND `animprogress` <=> 255 AND `state` <=> 1)
 AND (SELECT COUNT(*) FROM `gameobject_scripts` WHERE id=138378)=0
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=-138378 AND source_type=1)=0;
CREATE TEMPORARY TABLE fuzan_sanctuary_door (ok TINYINT NOT NULL, CONSTRAINT fuzan_sanctuary_door_native_guard CHECK(ok=1));
INSERT INTO fuzan_sanctuary_door VALUES(@fuzan_sanctuary_door_safe);
UPDATE gameobject_template SET ScriptName='go_fuzan_temple_sanctuary_door' WHERE entry=251591 AND @fuzan_sanctuary_door_safe;
DROP TEMPORARY TABLE fuzan_sanctuary_door;
COMMIT;
