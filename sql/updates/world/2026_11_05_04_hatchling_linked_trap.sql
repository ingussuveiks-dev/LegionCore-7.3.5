-- Both objects are native chest variants with distinct loot43369/43370.
-- Data7 is a trap entry, not a link to the larger quest-item chest.
UPDATE gameobject_template SET Data7=0
WHERE entry=214862 AND type=3 AND Data7=214863;
