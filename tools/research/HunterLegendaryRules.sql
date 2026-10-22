-- Read-only Legion 7.3.5 world database audit; no character or credential data.
SELECT * FROM spell_proc_event
WHERE entry IN (207280,212278,235721,248084);
SELECT * FROM spell_proc_check WHERE entry=207280;
SELECT * FROM spell_trigger
WHERE spell_id IN (212278,225139,241332);
SELECT * FROM spell_linked_spell
WHERE ABS(spell_trigger) IN (19574,207280,5384,208884,241332)
   OR ABS(spell_effect) IN (207318,208888);
SELECT * FROM spell_aura_trigger
WHERE spell_id IN (233665,207280,207318);
SELECT * FROM spell_aura_dummy
WHERE spellId=235999 OR spellDummyId IN (235991,236062);
SELECT * FROM spell_script_names
WHERE spell_id IN (207472,235169,257233,256832,256833);
-- Empty Pantheon bindings do not prove the whole item works. The dedicated
-- 257233 implementation in spell_item.cpp is currently commented out.
