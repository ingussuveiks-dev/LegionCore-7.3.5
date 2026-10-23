-- Only the native Titanstrike workshop uses these entries. Keep custom bindings.
UPDATE creature_template SET ScriptName='npc_titanstrike_xr947'
WHERE entry=105016 AND AIName='SmartAI' AND ScriptName='';
SET @titanstrike_workshop = EXISTS (SELECT 1 FROM creature_template WHERE entry=105016 AND ScriptName='npc_titanstrike_xr947');

-- UPDATE has no Unit invoker, and a nearby-30 check missed the distant tank.
-- The delegate emits 2/2 only after boot and all three actual DB actors die.
UPDATE smart_scripts SET event_type=38,event_param1=2,event_param2=2,
 comment='Titanstrike - tracked workshop defeated - native functional path'
WHERE @titanstrike_workshop AND entryorguid=105016 AND source_type=0 AND id=2 AND action_type=53 AND action_param2=105016;
DELETE FROM conditions WHERE @titanstrike_workshop AND SourceTypeOrReferenceId=22 AND SourceEntry=105016 AND SourceGroup=3
 AND ConditionTypeOrReference=29 AND ConditionValue1 IN (106352,106354);
-- Delegate retries the hologram summon if it fails, without duplicate actors.
UPDATE smart_scripts SET action_type=45,action_param1=3,action_param2=3,
 action_param3=0,action_param4=0,action_param5=0,action_param6=0,
 target_type=1,target_x=0,target_y=0,target_z=0,target_o=0,
 comment='Titanstrike - console reached - reveal actual Mimiron hologram'
WHERE @titanstrike_workshop AND entryorguid=105016 AND source_type=0 AND id=6
 AND ((action_type=12 AND action_param1=106376) OR (action_type=45 AND action_param1=3));

-- The old single conversation addressed Mimiron's lines to a player sentinel
-- before the hologram existed, and expired before its final line. Split only at
-- that actor boundary, retaining all native line/sound IDs and timing offsets.
INSERT IGNORE INTO conversation_data (entry,id,idx,textId,unk1,unk2)
SELECT 9002055,id,idx-2,textId-14150,unk1,unk2
FROM conversation_data WHERE @titanstrike_workshop AND entry=2055 AND idx>=2 AND textId>=14150;
INSERT IGNORE INTO conversation_creature (entry,id,creatureId,creatureGuid,unk1,unk2,duration)
SELECT 9002055,id,IF(id=2,106376,creatureId),creatureGuid,unk1,unk2,IF(id=2,50000,0)
FROM conversation_creature WHERE @titanstrike_workshop AND entry=2055;
DELETE FROM conversation_data WHERE @titanstrike_workshop AND entry=2055 AND idx>=2
 AND EXISTS (SELECT 1 FROM conversation_data child WHERE child.entry=9002055 AND child.id=conversation_data.id);
UPDATE conversation_creature SET duration=IF(id=0,14150,0)
WHERE @titanstrike_workshop AND entry=2055;
