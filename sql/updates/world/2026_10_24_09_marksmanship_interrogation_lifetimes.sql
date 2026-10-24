START TRANSACTION;

SET @marksman_capture_lifetime :=
    (SELECT COUNT(*) FROM conversation_actor WHERE entry=1033)=2
AND EXISTS (SELECT 1 FROM conversation_actor WHERE entry=1033 AND id=0 AND actorId=50831 AND creatureId=101269
    AND displayId=67031 AND unk1=0 AND unk2=1 AND unk3=0 AND duration IN(9000,27000))
AND EXISTS (SELECT 1 FROM conversation_actor WHERE entry=1033 AND id=1 AND actorId=50643 AND creatureId=101115
    AND displayId=28222 AND unk1=0 AND unk2=1 AND unk3=0 AND duration=9000)
AND (SELECT COUNT(*) FROM conversation_data WHERE entry=1033)=3
AND EXISTS (SELECT 1 FROM conversation_data WHERE entry=1033 AND id=2539 AND idx=0 AND textId=0 AND unk1=627 AND unk2=0)
AND EXISTS (SELECT 1 FROM conversation_data WHERE entry=1033 AND id=2541 AND idx=1 AND textId=5500 AND unk1=121 AND unk2=1)
AND EXISTS (SELECT 1 FROM conversation_data WHERE entry=1033 AND id=2543 AND idx=2 AND textId=9000 AND unk1=627 AND unk2=0)
AND NOT EXISTS (SELECT 1 FROM conversation_creature WHERE entry=1033);

SET @marksman_inquisitor_lifetime :=
    (SELECT COUNT(*) FROM conversation_actor WHERE entry=1556)=1
AND EXISTS (SELECT 1 FROM conversation_actor WHERE entry=1556 AND id=0 AND actorId=50831 AND creatureId=101269
    AND displayId=67031 AND unk1=0 AND unk2=1 AND unk3=0 AND duration IN(3450,10000))
AND (SELECT COUNT(*) FROM conversation_data WHERE entry=1556)=1
AND EXISTS (SELECT 1 FROM conversation_data WHERE entry=1556 AND id=3422 AND idx=0 AND textId=0 AND unk1=627 AND unk2=0)
AND NOT EXISTS (SELECT 1 FROM conversation_creature WHERE entry=1556);

UPDATE conversation_actor SET duration=27000 WHERE entry=1033 AND id=0 AND @marksman_capture_lifetime;
UPDATE conversation_actor SET duration=10000 WHERE entry=1556 AND id=0 AND @marksman_inquisitor_lifetime;

COMMIT;
