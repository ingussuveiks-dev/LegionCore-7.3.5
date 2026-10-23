INSERT INTO gameobject_template (entry,type,displayId,name,IconName,size,ScriptName)
SELECT 9000575,10,8111,'Portal to Dalaran','interact',1,'go_talonclaw_return_portal'
WHERE NOT EXISTS (SELECT 1 FROM gameobject_template WHERE entry=9000575);

INSERT INTO creature_text (CreatureID,GroupID,ID,Text,Type,Language,Probability,Emote,Duration,Sound,BroadcastTextID,MinTimer,MaxTimer,SpellID,comment)
SELECT 100005,1,0,'Hero, I sense your destiny lies with the ancient night elf who started you on this journey. Before I depart, I can send you back to the city in the sky. May Talonclaw serve you well.',12,0,100,0,0,0,106397,0,0,0,'Dorro native departure speech'
WHERE NOT EXISTS (SELECT 1 FROM creature_text WHERE CreatureID=100005 AND GroupID=1);

UPDATE smart_scripts AS credit
JOIN smart_scripts AS actor ON actor.entryorguid=credit.entryorguid AND actor.source_type=credit.source_type AND actor.id=3
SET credit.link=0
WHERE credit.entryorguid=248834 AND credit.source_type=1 AND credit.id=1 AND credit.link=3
AND credit.event_type=61 AND credit.action_type=205 AND credit.action_param1=92 AND credit.action_param2=47689
AND actor.event_type=61 AND actor.action_type=12 AND actor.action_param1=100005
AND EXISTS (SELECT 1 FROM gameobject_template WHERE entry=9000575 AND ScriptName='go_talonclaw_return_portal')
AND EXISTS (SELECT 1 FROM creature_text WHERE CreatureID=100005 AND GroupID=1 AND BroadcastTextID=106397);

DELETE actor FROM smart_scripts AS actor
JOIN smart_scripts AS credit ON credit.entryorguid=actor.entryorguid AND credit.source_type=actor.source_type AND credit.id=1
WHERE actor.entryorguid=248834 AND actor.source_type=1 AND actor.id=3
AND actor.event_type=61 AND actor.action_type=12 AND actor.action_param1=100005
AND credit.link=0 AND credit.event_type=61 AND credit.action_type=205 AND credit.action_param1=92 AND credit.action_param2=47689
AND EXISTS (SELECT 1 FROM gameobject_template WHERE entry=9000575 AND ScriptName='go_talonclaw_return_portal')
AND EXISTS (SELECT 1 FROM creature_text WHERE CreatureID=100005 AND GroupID=1 AND BroadcastTextID=106397);
