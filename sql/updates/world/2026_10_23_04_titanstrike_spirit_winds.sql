-- Native Spirit Winds had visuals/box/force data, but no target-tracking action.
-- The scoped AI owns force removal/retry across overlapping boxes and vehicles.
INSERT INTO areatrigger_scripts (entry, ScriptName)
SELECT 11360, 'at_titanstrike_spirit_winds'
WHERE NOT EXISTS (SELECT 1 FROM areatrigger_scripts WHERE entry=11360);
INSERT INTO areatrigger_actions
 (entry,customEntry,id,moment,actionType,targetFlags,spellId,comment)
SELECT 6837,11360,0,1,15,8,210880,'Spirit Winds - track players for scoped movement force AI'
WHERE EXISTS (SELECT 1 FROM areatrigger_scripts WHERE entry=11360 AND ScriptName='at_titanstrike_spirit_winds')
AND NOT EXISTS (SELECT 1 FROM areatrigger_actions WHERE entry=6837 AND customEntry=11360 AND id=0);
-- The native loader validates spellId even for NO_ACTION; repair an early
-- already-applied tracking row without changing a customized action.
UPDATE areatrigger_actions SET spellId=210880
WHERE entry=6837 AND customEntry=11360 AND id=0 AND moment=1
AND actionType=15 AND targetFlags=8 AND spellId=0;

-- A timer has no Unit invoker: inspect the actual ritual emitter as the origin.
UPDATE conditions SET ConditionTarget=1
WHERE SourceTypeOrReferenceId=22 AND SourceEntry=64367 AND SourceGroup=1
AND ConditionTypeOrReference=29 AND ConditionValue1=104949 AND ConditionTarget=0;
