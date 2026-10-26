UPDATE creature AS spawn
JOIN creature_template AS template ON template.entry=spawn.id
LEFT JOIN creature_addon AS addon ON addon.guid=spawn.guid
LEFT JOIN creature_template_addon AS inherited ON inherited.entry=spawn.id
SET spawn.MovementType=0
WHERE spawn.map=1539 AND spawn.MovementType=2 AND spawn.spawndist=0 AND template.MovementType=0
  AND COALESCE(addon.path_id,inherited.path_id,0)=0
  AND (
      (spawn.guid=361895 AND spawn.id=106429 AND spawn.PhaseId='5738 7005' AND template.ScriptName='npc_travar_first')
      OR (spawn.guid=361991 AND spawn.id=106639 AND spawn.PhaseId='7000' AND template.ScriptName='npc_travar_second')
      OR (spawn.guid=362006 AND spawn.id=106676 AND spawn.PhaseId='7002' AND template.ScriptName='npc_travar_third')
      OR (spawn.guid IN (361893,361986) AND spawn.id=106371 AND template.ScriptName='npc_efrin_and_duval'
          AND ((spawn.guid=361893 AND spawn.PhaseId='5738 7005') OR (spawn.guid=361986 AND spawn.PhaseId='7000')))
      OR (spawn.guid IN (361894,361987) AND spawn.id=106370 AND template.ScriptName='npc_efrin_and_duval'
          AND ((spawn.guid=361894 AND spawn.PhaseId='5738 7005') OR (spawn.guid=361987 AND spawn.PhaseId='7000')))
  );
