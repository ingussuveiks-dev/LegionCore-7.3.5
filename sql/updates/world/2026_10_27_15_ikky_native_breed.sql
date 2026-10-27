-- Ikky has one P/S breed: native BattlePetBreedState ID 8, not H/S ID 9.
-- Preserve custom masks and every other template field. No journal/progress edits.
UPDATE battlepet_info SET breadsMask=256
WHERE NpcID=86447 AND Specie=1532 AND breadsMask=512;
