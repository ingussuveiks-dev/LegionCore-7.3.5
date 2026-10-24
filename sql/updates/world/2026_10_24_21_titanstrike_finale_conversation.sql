START TRANSACTION;

SET @titanstrike_native_finale :=
    (SELECT COUNT(*) FROM conversation_actor WHERE entry=2147)=3
    AND (SELECT COUNT(*) FROM conversation_actor WHERE entry=2147
         AND unk1=0 AND unk2=1 AND unk3=0
         AND ((id=0 AND actorId=52951 AND creatureId=106748 AND displayId=69706 AND duration IN(3550,15000))
           OR (id=1 AND actorId=52944 AND creatureId=106714 AND displayId=69753 AND duration=3550)
           OR (id=2 AND actorId=52946 AND creatureId=106715 AND displayId=66838 AND duration=3550)))=3
    AND NOT EXISTS (SELECT 1 FROM conversation_creature WHERE entry=2147)
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=2147)=3
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=2147
         AND ((id=4555 AND idx=0 AND textId=0 AND unk1=137 AND unk2=0)
           OR (id=4556 AND idx=1 AND textId=7800 AND unk1=599 AND unk2=1)
           OR (id=4557 AND idx=2 AND textId=15700 AND unk1=90 AND unk2=2)))=3;

UPDATE conversation_actor SET duration=15000
WHERE entry=2147 AND id=0 AND @titanstrike_native_finale;

COMMIT;
