-- 44771 A Threat Rises: fetch actual grog, then bring it to Havi.
-- Phase7510 was incorrectly conditional on BOTH44771 and unrelated42004.
-- Keep the other condition as an alternative instead of deleting its content.
UPDATE conditions SET ElseGroup=1
WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=168
  AND ConditionTypeOrReference=9 AND ConditionValue1=42004;

-- Keep the existing narration/path and44721 gossip. Neither listening nor
-- standing near another player's narration awards the grog objective.
DELETE FROM smart_scripts WHERE entryorguid=9253900 AND source_type=9 AND id IN(8,9);

-- The client has PlayerCondition45843 on the keg (rewarded44720, active44771,
-- no Holding Grog). Enforce the same acquisition gates server-side, including
-- the spell completing after abandonment. Do not replace Lock2617/229547's18s
-- cast with the unused15s229542, which also carries premature group credit.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry IN(229547,229539);
INSERT INTO conditions
(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES
(17,0,229547,0,0,8,0,44720,0,0,0,0,'','Pouring Grog - rewarded A Call to Action'),
(17,0,229547,0,0,9,0,44771,0,0,0,0,'','Pouring Grog - incomplete A Threat Rises'),
(17,0,229547,0,0,1,0,229539,0,0,1,0,'','Pouring Grog - no drink already held'),
(17,0,229539,0,0,8,0,44720,0,0,0,0,'','Holding Grog - rewarded A Call to Action'),
(17,0,229539,0,0,9,0,44771,0,0,0,0,'','Holding Grog - incomplete A Threat Rises'),
(17,0,229539,0,0,1,0,229539,0,0,1,0,'','Holding Grog - no drink already held');
