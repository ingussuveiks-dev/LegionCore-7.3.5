START TRANSACTION;

SET @marksman_native_farewell :=
    (SELECT COUNT(*) FROM conversation_creature WHERE entry=1072)=3
    AND EXISTS (SELECT 1 FROM conversation_creature WHERE entry=1072 AND id=0 AND creatureId=100397 AND duration IN(2650,25000))
    AND EXISTS (SELECT 1 FROM conversation_creature WHERE entry=1072 AND id=1 AND creatureId=100398 AND duration=2650)
    AND EXISTS (SELECT 1 FROM conversation_creature WHERE entry=1072 AND id=2 AND creatureId=0 AND duration=2650)
    AND NOT EXISTS (SELECT 1 FROM conversation_actor WHERE entry=1072)
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=1072)=6
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=1072 AND unk1=0
         AND ((id=2625 AND idx=0 AND textId=0 AND unk2=0)
           OR (id=2630 AND idx=1 AND textId=4550 AND unk2=1)
           OR (id=2658 AND idx=2 AND textId=9850 AND unk2=0)
           OR (id=2659 AND idx=3 AND textId=13050 AND unk2=1)
           OR (id=3427 AND idx=4 AND textId=18050 AND unk2=258)
           OR (id=2631 AND idx=5 AND textId=23050 AND unk2=0)))=6;

UPDATE conversation_creature SET duration=25000
WHERE entry=1072 AND id=0 AND @marksman_native_farewell;

COMMIT;
