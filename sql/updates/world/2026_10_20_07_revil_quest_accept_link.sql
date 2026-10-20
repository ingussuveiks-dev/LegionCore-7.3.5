-- Revil's summoned clone starts its route via JUST_SUMMONED row0 ->1 ->7.
-- Quest acceptance already summons that clone before following this stale
-- reference. There is no row6 to execute; retain the native clone lifecycle.
UPDATE smart_scripts SET link=0
WHERE entryorguid=100812 AND source_type=0 AND id IN(4,5) AND event_type=19
 AND event_param1 IN(40623,40934) AND action_type=12 AND action_param1=100812
 AND link=6;
