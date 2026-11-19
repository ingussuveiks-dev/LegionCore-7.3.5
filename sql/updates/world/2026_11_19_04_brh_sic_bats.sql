-- 203163's actual native SCRIPT_EFFECT reaches bats;the matching EXE hands off the real marked player.
-- Keep both casts and all existing timers/targets;replace the dangling reference to nonexistent event2.
DROP TEMPORARY TABLE IF EXISTS combat_bats_guard;
CREATE TEMPORARY TABLE combat_bats_guard (valid TINYINT NOT NULL CHECK (valid=1));
INSERT INTO combat_bats_guard SELECT IF((SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=102788 AND source_type=0)=2 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=102788 AND source_type=0 AND id=1 AND link IN(0,2) AND event_type=0 AND action_type=11 AND action_param1=203163 AND target_type=5)=1 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=102788 AND source_type=0 AND id=2),1,0);
UPDATE smart_scripts SET link=0 WHERE entryorguid=102788 AND source_type=0 AND id=1 AND link=2 AND event_type=0 AND action_type=11 AND action_param1=203163 AND target_type=5;
DROP TEMPORARY TABLE combat_bats_guard;
