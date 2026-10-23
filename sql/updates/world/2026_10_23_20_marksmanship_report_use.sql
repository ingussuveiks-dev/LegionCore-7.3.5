UPDATE gameobject_template AS object
JOIN smart_scripts AS replay ON replay.entryorguid=object.entry AND replay.source_type=1 AND replay.id=2
SET object.ScriptName='go_marksmanship_pickup'
WHERE object.entry=248419 AND object.type=3 AND object.Data1=248419
  AND object.AIName='SmartGameObjectAI' AND object.ScriptName='go_artifact_acquisition'
  AND replay.event_type=64 AND replay.event_param1=3 AND replay.link=1
  AND replay.action_type=85 AND replay.action_param1=200279 AND replay.target_type=7;
