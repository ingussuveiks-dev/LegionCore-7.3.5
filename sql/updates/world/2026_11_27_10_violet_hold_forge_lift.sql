-- The native Activating spell summons this vehicle for the mage in Violet Hold.
UPDATE `creature_template`
SET `ScriptName` = 'npc_violet_hold_forge_lift'
WHERE `entry` = 102915 AND `VehicleId` = 4596 AND `AIName` = '' AND `ScriptName` = '';
