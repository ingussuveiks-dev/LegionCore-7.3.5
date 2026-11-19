-- The area entry roots use EventObject SmartScripts (13).
-- Gate the original dialogue and credit chain at its first action.
UPDATE conditions SET SourceId=13
WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceId=10
  AND SourceEntry IN (199,234,236,250,322,323);

-- The completed 45065 introduction has one action and no authored continuation.
UPDATE smart_scripts AS root
LEFT JOIN smart_scripts AS next
  ON next.entryorguid=root.entryorguid AND next.source_type=root.source_type AND next.id=root.link
SET root.link=0
WHERE root.entryorguid=323 AND root.source_type=13 AND root.id=0 AND root.link=1
  AND root.event_type=89 AND root.action_type=206 AND root.action_param1=4024
  AND next.id IS NULL;
