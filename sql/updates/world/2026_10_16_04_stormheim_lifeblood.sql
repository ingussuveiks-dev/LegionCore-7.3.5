-- Paid in Lifeblood39855: the bound altar AI validates its actual current
-- option1 and credits only the individual player's acknowledged arrival.
-- Do not leave the legacy group-credit/linked teleport as an alternate path.
DELETE s FROM smart_scripts s JOIN gameobject_template g ON g.entry=s.entryorguid
WHERE g.ScriptName='go_stormheim_helheim_bargain' AND s.entryorguid=244559 AND s.source_type=1 AND
 ((s.id=3 AND s.event_type=62 AND s.event_param1=18790 AND s.event_param2=1 AND s.link=4 AND s.action_type=33 AND s.action_param1=97487 AND s.target_type=7) OR
  (s.id=4 AND s.link=0 AND s.event_type=61 AND s.action_type=62 AND s.action_param1=1220 AND s.target_type=7 AND ABS(s.target_x-3152.87)<0.01 AND ABS(s.target_y-1536.12)<0.01 AND ABS(s.target_z-181.33)<0.01));
UPDATE creature_template SET ScriptName='npc_stormheim_lifeblood_ashildir'
WHERE entry=97558 AND AIName IN('','SmartAI') AND ScriptName IN('','npc_stormheim_lifeblood_ashildir');
-- Source22 SmartAI conditions never gated a template spell cast. The scoped
-- IsTriggerMeets hook runs before241140 and enforces rewarded39855 here.
UPDATE eventobject_template SET ScriptName='eventobject_stormheim_lifeblood_return'
WHERE entry=333 AND SpellID=241140 AND WorldSafeLocID=0 AND radius=2 AND
 ScriptName IN('','SmartEventObject','eventobject_stormheim_lifeblood_return');
