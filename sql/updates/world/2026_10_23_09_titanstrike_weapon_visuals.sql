--42428 is the Infernal placeholder. Native display69591 is an invisible
-- anchor whose state kit67322 attaches the actual artifact rifle1346250.
UPDATE creature_template_wdb SET DisplayId1=69591,DisplayId2=0
WHERE Entry IN(106428,106745) AND DisplayId1=42428 AND DisplayId2=69591
 AND DisplayId3=0 AND DisplayId4=0;

-- Both are dedicated, elevated Titanstrike props, not flying combat actors.
-- CanFly(2) alone still permits gravity; keep their native summon height.
INSERT INTO creature_template_movement(CreatureId,Ground,Flight)
SELECT Entry,0,1 FROM creature_template_wdb
WHERE Entry IN(106428,106745) AND DisplayId1=69591
 AND DisplayId2=0 AND DisplayId3=0 AND DisplayId4=0
ON DUPLICATE KEY UPDATE Ground=VALUES(Ground),Flight=VALUES(Flight);

-- Prustaga already carries128861 in her ranged virtual-item slot. Draw it
-- exactly when the floating prop is taken, before the walk to her portal.
-- Guard the native timed list; preserve custom choreography and repeat runs.
SET @titanstrike_draw_rifle=(
 SELECT COUNT(*)=10 AND SUM(link<>0 OR event_type<>0 OR event_param1<>event_param2)=0 AND SUM(
 (id=0 AND action_type=11 AND action_param1=211083 AND event_param1=10000) OR
 (id=1 AND action_type=28 AND action_param1=0 AND event_param1=0) OR
 (id=2 AND action_type=53 AND action_param2=10494903 AND event_param1=0) OR
 (id=3 AND action_type=11 AND action_param1=196978 AND event_param1=6000) OR
 (id=4 AND action_type=45 AND action_param1=1 AND action_param2=1 AND target_type=19 AND target_param1=106428 AND event_param1=1500) OR
 (id=5 AND action_type=53 AND action_param2=10494904 AND event_param1=2000) OR
 (id=6 AND action_type=11 AND action_param1=196978 AND event_param1=5000) OR
 (id=7 AND action_type=50 AND action_param1=249736 AND event_param1=1000) OR
 (id=8 AND action_type=45 AND action_param1=1 AND action_param2=1 AND target_param1=106376 AND event_param1=3000) OR
 (id=9 AND action_type=41 AND event_param1=0))=10
 FROM smart_scripts WHERE entryorguid=10494902 AND source_type=9
) AND EXISTS(SELECT 1 FROM creature_equip_template
 WHERE CreatureID=104949 AND ID=1 AND ItemID5=128861);
UPDATE smart_scripts SET id=id+1
WHERE entryorguid=10494902 AND source_type=9 AND id>=5
 AND @titanstrike_draw_rifle ORDER BY id DESC;
INSERT INTO smart_scripts
 (entryorguid,source_type,id,event_type,event_chance,action_type,action_param1,target_type,comment)
SELECT 10494902,9,5,0,100,40,2,1,
 'Prustaga - after taking Titanstrike - draw ranged weapon before leaving'
WHERE @titanstrike_draw_rifle;
