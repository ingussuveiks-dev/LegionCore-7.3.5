-- The Blessing of the Watchers: speak when ready, receive the actual effect,
-- then gain the original 96285 objective. Preserve shared SmartAI for reuse.
-- Original 7.3.5 reward: one artifact-power item, not an invented direct grant.
UPDATE quest_template SET RewardItem1=141398,RewardAmount1=1
WHERE ID=39597 AND RewardItem1=0 AND RewardAmount1=0;
UPDATE creature_template SET npcflag=3,gossip_menu_id=18672
WHERE entry=96257 AND npcflag=2 AND gossip_menu_id=0 AND AIName IN ('','SmartAI')
  AND ScriptName IN ('','npc_stormheim_blessing');
UPDATE creature_template SET AIName='',ScriptName='npc_stormheim_blessing'
WHERE entry IN(96257,96258) AND npcflag=3 AND gossip_menu_id=18672
  AND AIName IN ('','SmartAI') AND ScriptName IN ('','npc_stormheim_blessing');

INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 15,18672,0,0,0,9,0,39597,0,0,0,0,'','Stormheim - blessing readiness while quest incomplete'
WHERE NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18672 AND SourceEntry=0);

INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 190905,'aura_stormheim_blessing_finish'
WHERE NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=190905 AND ScriptName='aura_stormheim_blessing_finish');
