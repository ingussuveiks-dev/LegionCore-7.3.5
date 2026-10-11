-- Riverbend: native horn evacuation and actual Grasp of Dargrul passenger release.
UPDATE creature_template SET AIName='',ScriptName='npc_riverbend_evacuee'
WHERE entry=100520 AND ScriptName IN('','npc_riverbend_evacuee');
DELETE FROM smart_scripts WHERE entryorguid=100520 AND source_type=0 AND id IN(0,1,2) AND event_type=8 AND event_param1=210891;
-- The grasp AI owns boarding/recovery. These competing timers could reseat a released captive.
DELETE FROM smart_scripts WHERE entryorguid IN(94255,95051,94286) AND source_type=0 AND id=4
 AND event_type=60 AND action_type=11 AND action_param1=46598 AND target_type=11 AND target_param1=97648;
-- TDB735 objective records: three drogbar and three basilisks, not five each.
UPDATE quest_objectives SET Amount=3 WHERE ID IN(283671,283672) AND QuestID=39043 AND Type=0 AND ObjectID IN(94261,98653);
