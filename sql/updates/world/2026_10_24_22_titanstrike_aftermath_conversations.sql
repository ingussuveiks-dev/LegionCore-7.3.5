START TRANSACTION;

SET @titanstrike_native_aftermath :=
    (SELECT COUNT(*) FROM conversation_actor WHERE entry=2167)=3
    AND (SELECT COUNT(*) FROM conversation_actor WHERE entry=2167 AND unk1=0 AND unk2=1 AND unk3=0
         AND ((id=0 AND actorId=52946 AND creatureId=106715 AND displayId=66838 AND duration IN(9400,50000))
           OR (id=1 AND actorId=52943 AND creatureId=106713 AND displayId=28578 AND duration=9400)
           OR (id=2 AND actorId=52944 AND creatureId=106714 AND displayId=69753 AND duration=9400)))=3
    AND NOT EXISTS (SELECT 1 FROM conversation_creature WHERE entry=2167)
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=2167)=5
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=2167
         AND ((id=4598 AND idx=0 AND textId=0 AND unk1=91 AND unk2=0)
           OR (id=4599 AND idx=1 AND textId=6600 AND unk1=621 AND unk2=1)
           OR (id=4600 AND idx=2 AND textId=16900 AND unk1=621 AND unk2=1)
           OR (id=4601 AND idx=3 AND textId=31550 AND unk1=598 AND unk2=2)
           OR (id=4826 AND idx=4 AND textId=44200 AND unk1=621 AND unk2=1)))=5;

UPDATE conversation_actor SET duration=50000
WHERE entry=2167 AND id=0 AND @titanstrike_native_aftermath;

SET @titanstrike_native_restoration :=
    (SELECT COUNT(*) FROM conversation_actor WHERE entry=2173)=2
    AND (SELECT COUNT(*) FROM conversation_actor WHERE entry=2173 AND unk1=0 AND unk2=1 AND unk3=0
         AND ((id=0 AND actorId=52944 AND creatureId=106714 AND displayId=69753 AND duration IN(3700,55000))
           OR (id=1 AND actorId=52946 AND creatureId=106715 AND displayId=66838 AND duration=3700)))=2
    AND NOT EXISTS (SELECT 1 FROM conversation_creature WHERE entry=2173)
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=2173)=5
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=2173
         AND ((id=4615 AND idx=0 AND textId=0 AND unk1=598 AND unk2=0)
           OR (id=4619 AND idx=1 AND textId=6250 AND unk1=598 AND unk2=0)
           OR (id=4616 AND idx=2 AND textId=17100 AND unk1=598 AND unk2=0)
           OR (id=4620 AND idx=3 AND textId=28000 AND unk1=91 AND unk2=1)
           OR (id=4621 AND idx=4 AND textId=38550 AND unk1=91 AND unk2=1)))=5;

UPDATE conversation_actor SET duration=55000
WHERE entry=2173 AND id=0 AND @titanstrike_native_restoration;

COMMIT;
