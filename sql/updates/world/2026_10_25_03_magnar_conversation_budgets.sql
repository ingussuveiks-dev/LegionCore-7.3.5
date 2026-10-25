START TRANSACTION;

SET @magnar_duel_native :=
    (SELECT COUNT(*) FROM conversation_creature WHERE entry=498)=2
    AND (SELECT COUNT(*) FROM conversation_creature WHERE entry=498
         AND unk1=2607677440 AND unk2=110
         AND ((id=0 AND creatureId=96034 AND creatureGuid=7868842 AND duration IN(4300,26000))
           OR (id=1 AND creatureId=96468 AND creatureGuid=7868614 AND duration=4300)))=2
    AND NOT EXISTS (SELECT 1 FROM conversation_actor WHERE entry=498)
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=498)=3
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=498 AND unk1=0
         AND ((id=1335 AND idx=0 AND textId=0 AND unk2=0)
           OR (id=1336 AND idx=1 AND textId=2350 AND unk2=1)
           OR (id=1337 AND idx=2 AND textId=7300 AND unk2=0)))=3;

SET @magnar_finale_native :=
    (SELECT COUNT(*) FROM conversation_creature WHERE entry=504)=2
    AND (SELECT COUNT(*) FROM conversation_creature WHERE entry=504
         AND unk1=2616135424 AND unk2=57
         AND ((id=0 AND creatureId=96034 AND creatureGuid=7868842 AND duration IN(7550,42000))
           OR (id=1 AND creatureId=96468 AND creatureGuid=7868614 AND duration=7550)))=2
    AND NOT EXISTS (SELECT 1 FROM conversation_actor WHERE entry=504)
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=504)=4
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=504 AND unk1=0
         AND ((id=1341 AND idx=0 AND textId=0 AND unk2=0)
           OR (id=1342 AND idx=1 AND textId=3950 AND unk2=1)
           OR (id=1343 AND idx=2 AND textId=12900 AND unk2=0)
           OR (id=1344 AND idx=3 AND textId=21350 AND unk2=1)))=4;

UPDATE conversation_creature SET duration=26000
WHERE entry=498 AND id=0 AND @magnar_duel_native;
UPDATE conversation_creature SET duration=42000
WHERE entry=504 AND id=0 AND @magnar_finale_native;

COMMIT;
