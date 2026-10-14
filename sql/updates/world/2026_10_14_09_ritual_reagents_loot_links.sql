-- Ritual Reagents (42128): these chests already have native quest loot rows,
-- but chestLoot (Data1) was zero, so Player::SendLoot skipped FillLoot entirely.
-- Keep quest requirements, personal loot settings, phases and visuals intact.
UPDATE gameobject_template SET Data1=entry
WHERE type=3 AND entry IN (249742,249744,249748) AND Data1=0;
