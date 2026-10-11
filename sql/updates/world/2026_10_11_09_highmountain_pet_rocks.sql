-- Pet Rocks 39391: Frag must credit on the actual 189687 spell hit.
-- Event 73 interprets parameter 1 as a shared cooldown, not a spell ID.
UPDATE smart_scripts SET event_type=8,event_param1=189687,event_param2=0,event_param3=0,event_param4=0,event_param5=0,
 comment='Frag - On Recovering hit - Credit the inspecting player (no shared cooldown)'
 WHERE entryorguid=95272 AND source_type=0 AND id=0 AND action_type=33 AND action_param1=95272;
-- Only quest participants should receive the rescue/inspection interaction.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=18 AND ((SourceGroup=95272 AND SourceEntry=189687) OR (SourceGroup IN(95273,95276) AND SourceEntry=189686));
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES
(18,95272,189687,0,0,9,0,39391,0,0,0,0,'','Frag inspection requires Pet Rocks'),
(18,95273,189686,0,0,9,0,39391,0,0,0,0,'','Shard rescue requires Pet Rocks'),
(18,95276,189686,0,0,9,0,39391,0,0,0,0,'','Gravel rescue requires Pet Rocks');
