-- Per-player altar transaction and native challenger spell; no shared cooldown.
-- TDB735: AllowMultiInteract=1. Keep the existing native summon spell 192443.
UPDATE gameobject_template SET AIName='',ScriptName='go_blood_trial_altar',Data20=1 WHERE entry=244564;
-- TDB735 creature scaling expansion is Legion, not Classic. Keep the existing
-- valid 98..110 scaling range and Burning Slash creature spell.
UPDATE creature_template SET AIName='',ScriptName='npc_blood_trial_warmonger',HealthScalingExpansion=6 WHERE entry=97544;
-- Restore the unreadable fallback from existing hotfix BroadcastText 99485.
UPDATE creature_text SET Text='A challenger? Let''s see what this one is capable of!'
WHERE CreatureID=97544 AND GroupID=1 AND ID=0 AND BroadcastTextID=99485;
