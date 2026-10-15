-- Stealth by Seaweed38347: personal guide and actual disguised exit334.
-- Retire only the old disguise-credit shortcuts; preserve other Guarm SAI
-- and Ashildir's native dialogue/39848 acceptance actions.
UPDATE creature_template SET AIName='',ScriptName='npc_stormheim_helheim_ashildir'
WHERE entry=97419 AND AIName IN('','SmartAI') AND ScriptName IN('','npc_stormheim_helheim_ashildir');
UPDATE eventobject_template SET ScriptName='eventobject_stormheim_helheim_passage'
WHERE entry=334 AND radius=4 AND SpellID=0 AND WorldSafeLocID=0
  AND ScriptName IN('','SmartEventObject','eventobject_stormheim_helheim_passage')
  AND EXISTS(SELECT 1 FROM eventobject WHERE guid=400326 AND id=334 AND map=1463);
-- Essential disguise no longer depends on the controller cast's LINK.
-- 192299 also triggers deleted81040; the new private wild guide does not
-- occupy a native guardian/summon slot or replace the player's primary pet.
DELETE s FROM smart_scripts s JOIN creature_template t ON t.entry=97419
JOIN eventobject_template e ON e.entry=334
WHERE t.ScriptName='npc_stormheim_helheim_ashildir' AND e.ScriptName='eventobject_stormheim_helheim_passage'
  AND s.entryorguid=91531 AND s.source_type=0 AND
  ((s.id=0 AND s.link=1 AND s.event_type=19 AND s.event_param1=38347 AND s.action_type=85 AND s.action_param1=192299 AND s.target_type=7)
   OR (s.id=1 AND s.link=0 AND s.event_type=61 AND s.action_type=85 AND s.action_param1=181705 AND s.target_type=7));
-- Old action33 also emitted kill achievement credit without an actual kill.
DELETE s FROM smart_scripts s JOIN creature_template t ON t.entry=97419
JOIN eventobject_template e ON e.entry=334
WHERE t.ScriptName='npc_stormheim_helheim_ashildir' AND e.ScriptName='eventobject_stormheim_helheim_passage'
  AND s.source_type=0 AND
  ((s.entryorguid=91386 AND s.id=0 AND s.event_type=10 AND s.action_type=33 AND s.action_param1=91386 AND s.target_type=7)
   OR (s.entryorguid=97419 AND s.id=1 AND s.event_type=38 AND s.event_param1=1 AND s.event_param2=1
       AND s.action_type=33 AND s.action_param1=91386 AND s.target_type=21 AND s.target_param1=10));
DELETE c FROM conditions c JOIN creature_template t ON t.entry=97419
JOIN eventobject_template e ON e.entry=334
WHERE t.ScriptName='npc_stormheim_helheim_ashildir' AND e.ScriptName='eventobject_stormheim_helheim_passage'
  AND c.SourceTypeOrReferenceId=22 AND c.SourceEntry=91386 AND c.SourceId=0 AND c.SourceGroup=1
  AND c.ConditionTypeOrReference=1 AND c.ConditionTarget=0 AND c.ConditionValue1=181705 AND c.NegativeCondition=0;
-- Retire the unowned SetData -> nearest-player relay, not other actor events.
DELETE s FROM smart_scripts s JOIN eventobject_template e ON e.entry=s.entryorguid
WHERE e.ScriptName='eventobject_stormheim_helheim_passage' AND s.entryorguid=334 AND s.source_type=13
  AND s.id=0 AND s.event_type=89 AND s.action_type=45 AND s.action_param1=1 AND s.action_param2=1
  AND s.target_type=11 AND s.target_param1=97419 AND s.target_param2=50;
-- This condition was keyed to SCENE10, whereas its old event was EVENTOBJECT13.
-- The replacement C++ event performs live quest/phase/position/aura checks.
DELETE c FROM conditions c JOIN eventobject_template e ON e.entry=c.SourceEntry
WHERE e.ScriptName='eventobject_stormheim_helheim_passage' AND c.SourceTypeOrReferenceId=22
  AND c.SourceEntry=334 AND c.SourceGroup=1 AND c.SourceId=10 AND c.ElseGroup=0
  AND c.ConditionTypeOrReference=9 AND c.ConditionTarget=0 AND c.ConditionValue1=38347 AND c.NegativeCondition=0;
-- Avoid SmartAI's empty-script admission error after retiring the last rows.
UPDATE creature_template c SET c.AIName=''
WHERE c.entry IN(91531,91386) AND c.AIName='SmartAI' AND c.ScriptName=''
  AND NOT EXISTS(SELECT 1 FROM smart_scripts s WHERE s.entryorguid=c.entry AND s.source_type=0);
