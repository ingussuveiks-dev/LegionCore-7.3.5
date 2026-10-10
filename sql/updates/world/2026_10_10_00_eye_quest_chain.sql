-- Credit entry only inside instance 1456, in its OnPlayerEnter hook.
-- The old shared template script also ran on the copy outside the dungeon.
DELETE FROM smart_scripts WHERE entryorguid=106847 AND source_type=0 AND id=0
AND event_type=60 AND action_type=33 AND action_param1=106847;

-- Standing near either credit bunny is not using the central teleporter.
-- Preserve the independent 109750 action for the unrelated 40890 quest.
DELETE FROM smart_scripts WHERE entryorguid=106815 AND source_type=0 AND id=0
AND event_type=60 AND action_type=33 AND action_param1=106815;
UPDATE gameobject_template SET ScriptName='go_eye_portrait_teleporter'
WHERE entry IN(244534,244560) AND ScriptName IN('','go_eye_portrait_teleporter');

-- TDB 7.3.5: the dungeon quest gives Tidestone Sliver, not the belt package
-- belonging to the subsequent Dalaran placement quest.
UPDATE quest_template SET QuestPackageID=0,RewardItem1=141385,RewardAmount1=1
WHERE ID=38286 AND QuestPackageID=18462 AND RewardItem1=0 AND RewardAmount1=0;
