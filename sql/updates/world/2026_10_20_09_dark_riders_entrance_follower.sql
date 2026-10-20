-- Preserve the Player-only2-yard stair introduction and its full5-second
-- TEXT_OVER. Its completion requests an instance-owned follower handoff;
-- a lost NPC invoker or rejected normal cast must not consume the only try.
UPDATE smart_scripts SET action_type=34,action_param1=10085002,action_param2=1,
 action_param3=0,action_param4=0,action_param5=0,action_param6=0,
 target_type=1,target_param1=0,target_param2=0,target_param3=0,target_param4=0,
 comment='Dark Riders entrance - completed introduction requests verified follower handoff'
WHERE entryorguid=-369378 AND source_type=0 AND id=1 AND event_type=52 AND link=2;
