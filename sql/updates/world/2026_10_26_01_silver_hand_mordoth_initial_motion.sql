UPDATE creature AS spawn
JOIN creature_template AS template ON template.entry=spawn.id
LEFT JOIN creature_addon AS addon ON addon.guid=spawn.guid
LEFT JOIN creature_template_addon AS inherited ON inherited.entry=spawn.id
SET spawn.MovementType=0
WHERE spawn.guid=362044 AND spawn.id=106719 AND spawn.map=1539 AND spawn.PhaseId='7001'
  AND spawn.MovementType=2 AND spawn.spawndist=0 AND template.MovementType=0
  AND template.AIName='SmartAI' AND (template.ScriptName IS NULL OR template.ScriptName='')
  AND COALESCE(addon.path_id,inherited.path_id,0)=0;
