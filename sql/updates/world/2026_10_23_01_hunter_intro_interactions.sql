-- Complete the existing Hunter's Call credit -> LINK interaction. Do not
-- overwrite a custom continuation, or remove the original credit action.
INSERT INTO `smart_scripts`
 (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_chance`,
  `action_type`,`target_type`,`comment`)
SELECT s.`entryorguid`,0,1,0,61,100,72,7,
       'Hunter introduction - after weapon information credit - close gossip'
FROM `smart_scripts` s
LEFT JOIN `smart_scripts` next ON next.`entryorguid`=s.`entryorguid`
 AND next.`source_type`=0 AND next.`id`=1
WHERE s.`source_type`=0 AND s.`id`=0 AND s.`link`=1
 AND s.`event_type`=62 AND s.`action_type`=33 AND s.`target_type`=7
 AND ((s.`entryorguid`=100810 AND s.`action_param1`=104297)
   OR (s.`entryorguid`=99986 AND s.`action_param1`=104298)
   OR (s.`entryorguid`=100171 AND s.`action_param1`=104299))
 AND next.`id` IS NULL;

-- Spellclick admission runs BEFORE Huey's summon spell and SmartAI. The
-- outgoing and final-return uses share this bird: allow active/complete
-- Stolen Thunder, retaining the native scenario-1099 final return branch.
-- Source18 uses NPC as SourceGroup and spell as SourceEntry.
INSERT INTO `conditions`
 (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,
  `ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`Comment`)
SELECT 18,106854,212060,0,g.elseGroup,g.conditionType,0,41574,
       'Huey - Stolen Thunder must be active or complete before spellclick'
FROM (SELECT 0 AS elseGroup,9 AS conditionType UNION ALL SELECT 1,28) g
WHERE NOT EXISTS (SELECT 1 FROM `conditions` c
 WHERE c.`SourceTypeOrReferenceId`=18 AND c.`SourceGroup`=106854
 AND c.`SourceEntry`=212060 AND c.`SourceId`=0);
