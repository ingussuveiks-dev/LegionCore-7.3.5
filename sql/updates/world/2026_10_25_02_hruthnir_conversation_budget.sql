START TRANSACTION;

SET @hruthnir_native_dialogue :=
    (SELECT COUNT(*) FROM conversation_creature WHERE entry=497)=3
    AND (SELECT COUNT(*) FROM conversation_creature WHERE entry=497
         AND unk1=2616144384 AND unk2=32554
         AND ((id=0 AND creatureId=96468 AND creatureGuid=7868614 AND duration IN(0,68000))
           OR (id=1 AND creatureId=96034 AND creatureGuid=7868842 AND duration=0)
           OR (id=2 AND creatureId=0 AND creatureGuid=4294967295 AND duration=0)))=3
    AND NOT EXISTS (SELECT 1 FROM conversation_actor WHERE entry=497)
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=497)=10
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=497 AND unk1=0
         AND ((id=1324 AND idx=0 AND textId=0 AND unk2=0)
           OR (id=1325 AND idx=1 AND textId=7000 AND unk2=1)
           OR (id=1331 AND idx=2 AND textId=10000 AND unk2=1)
           OR (id=1326 AND idx=3 AND textId=12000 AND unk2=0)
           OR (id=1327 AND idx=4 AND textId=19000 AND unk2=1)
           OR (id=1328 AND idx=5 AND textId=22000 AND unk2=0)
           OR (id=1329 AND idx=6 AND textId=30000 AND unk2=1)
           OR (id=1330 AND idx=7 AND textId=32000 AND unk2=0)
           OR (id=1332 AND idx=8 AND textId=40000 AND unk2=1)
           OR (id=1334 AND idx=9 AND textId=41000 AND unk2=258)))=10;

UPDATE conversation_creature SET duration=68000
WHERE entry=497 AND id=0 AND @hruthnir_native_dialogue;

COMMIT;
