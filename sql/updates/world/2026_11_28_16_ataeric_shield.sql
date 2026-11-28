-- 27518 advances from the Arcane Expulsion energy carried by real fiend deaths.
UPDATE creature_template SET AIName='', ScriptName='npc_silverpine_ataeric'
WHERE entry=2120 AND AIName='SmartAI' AND ScriptName='';

DELETE FROM smart_scripts
WHERE source_type=0 AND entryorguid=2120
  AND ((id=0 AND link=1 AND event_type=25 AND action_type=11 AND action_param1=87251)
    OR (id=1 AND link=2 AND event_type=0 AND action_type=11 AND action_param1=87253)
    OR (id=2 AND link=3 AND event_type=0 AND action_type=37 AND event_param1=120000 AND event_param2=120000)
    OR (id=3 AND link=0 AND event_type=61 AND action_type=33 AND action_param1=45769));

-- The original owner now invokes this spell once with the actual death receipt.
DELETE FROM smart_scripts
WHERE entryorguid=45766 AND source_type=0 AND id=0 AND link=0 AND event_type=6
  AND action_type=11 AND action_param1=87249 AND target_type=1;

INSERT IGNORE INTO spell_script_names (spell_id, ScriptName)
VALUES (87249, 'spell_silverpine_arcane_expulsion');

INSERT IGNORE INTO creature_text
    (CreatureID, GroupID, ID, Text, Type, Language, Probability, BroadcastTextId, Comment)
VALUES
    (2120,0,0,'',14,0,100,45824,'Ataeric - begin shield defense'),
    (2120,1,0,'',14,0,100,45825,'Ataeric - shield at 20 percent'),
    (2120,2,0,'',14,0,100,45826,'Ataeric - shield at 40 percent'),
    (2120,3,0,'',14,0,100,45827,'Ataeric - shield at 60 percent'),
    (2120,4,0,'',14,0,100,45828,'Ataeric - shield at 80 percent'),
    (2120,5,0,'',14,0,100,46922,'Ataeric - shield overloaded'),
    (2120,6,0,'',14,0,100,45829,'Ataeric - final warning'),
    (2120,7,0,'',41,0,100,46925,'Ataeric - fiend proximity hint');
