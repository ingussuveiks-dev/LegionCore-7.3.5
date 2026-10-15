-- These are EventObject SmartScripts (13), not Scene scripts (10).
DELETE c FROM conditions c JOIN conditions old
 ON old.SourceTypeOrReferenceId=c.SourceTypeOrReferenceId
 AND old.SourceGroup=c.SourceGroup AND old.SourceEntry=c.SourceEntry
 AND old.ElseGroup=c.ElseGroup AND old.ConditionTypeOrReference=c.ConditionTypeOrReference
 AND old.ConditionTarget=c.ConditionTarget AND old.ConditionValue1=c.ConditionValue1
 AND old.ConditionValue2=c.ConditionValue2 AND old.ConditionValue3=c.ConditionValue3
 AND old.NegativeCondition=c.NegativeCondition
 WHERE c.SourceTypeOrReferenceId=22 AND c.SourceId=13 AND old.SourceId=10
 AND c.SourceGroup=1 AND c.SourceEntry IN(124,134,145,147,148,158,165,167,177,181,198);
UPDATE conditions SET SourceId=13
 WHERE SourceTypeOrReferenceId=22 AND SourceId=10 AND SourceGroup=1
 AND SourceEntry IN(124,134,145,147,148,158,165,167,177,181,198);
DELETE FROM smart_scripts WHERE source_type=13 AND id=1
 AND entryorguid IN(124,134,145,147,148,158,165,167,177,181,198);
INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,event_chance,action_type,action_param1,target_type,comment)
 SELECT entry,13,1,90,100,28,219567,7,'Commander Valor - leave altar circle - remove extra action'
 FROM eventobject_template WHERE entry IN(124,134,145,147,148,158,165,167,177,181,198);
DELETE FROM spell_script_names WHERE ScriptName IN('spell_commanders_valor_location','aura_commanders_valor_location');
INSERT INTO spell_script_names(spell_id,ScriptName) VALUES
 (219565,'spell_commanders_valor_location'),(219567,'aura_commanders_valor_location');
