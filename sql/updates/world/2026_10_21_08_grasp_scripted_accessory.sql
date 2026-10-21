-- Riverbend's current quest AI creates and tracks the correct captive for each
-- Grasp spawn itself. The obsolete generic 95051 accessory used to fail boarding;
-- fixing native accessory boarding must not introduce an untracked/wrong captive.
DELETE a FROM vehicle_template_accessory a
JOIN creature_template c ON c.entry=a.EntryOrAura
WHERE a.EntryOrAura=97648 AND a.accessory_entry=95051 AND a.seat_id=0
  AND c.ScriptName='npc_grasp_of_underking_quest';
