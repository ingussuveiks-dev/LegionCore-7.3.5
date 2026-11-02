-- Matching26972 enUS complete voices plus native AdditionalDuration gaps.
-- Only exact original/already-repaired definitions; preserve custom content.
START TRANSACTION;

SET @warrior_voice_770 :=
    NOT EXISTS (SELECT 1 FROM conversation_creature WHERE entry=770)
    AND (SELECT COUNT(*) FROM conversation_actor WHERE entry=770)=1
    AND (SELECT COUNT(*) FROM conversation_actor WHERE entry=770 AND id=0 AND actorId=50342 AND creatureId=97167
         AND displayId=67919 AND unk1=0 AND unk2=1 AND unk3=0 AND duration IN(3250,5020))=1
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=770)=1
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=770 AND id=1831 AND idx=0 AND textId=0 AND unk1=135 AND unk2=0)=1;

SET @warrior_voice_1386 :=
    NOT EXISTS (SELECT 1 FROM conversation_actor WHERE entry=1386)
    AND (SELECT COUNT(*) FROM conversation_creature WHERE entry=1386)=2
    AND (SELECT COUNT(*) FROM conversation_creature WHERE entry=1386 AND unk1=0 AND unk2=4294967289
         AND ((id=0 AND creatureId=97166 AND creatureGuid=174984 AND duration=5200)
           OR (id=1 AND creatureId=97167 AND creatureGuid=174984 AND duration IN(5200,8756))))=2
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=1386)=2
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=1386 AND unk1=32612
         AND ((id=3171 AND idx=0 AND textId=0 AND unk2=0)
           OR (id=3172 AND idx=1 AND textId IN(3950,4376) AND unk2=1)))=2;

SET @warrior_voice_1405 :=
    NOT EXISTS (SELECT 1 FROM conversation_actor WHERE entry=1405)
    AND (SELECT COUNT(*) FROM conversation_creature WHERE entry=1405)=2
    AND (SELECT COUNT(*) FROM conversation_creature WHERE entry=1405 AND unk1=0 AND unk2=4294967289
         AND ((id=0 AND creatureId=103608 AND creatureGuid=175242 AND duration=4000)
           OR (id=1 AND creatureId=97167 AND creatureGuid=174984 AND duration IN(4000,14285))))=2
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=1405)=2
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=1405 AND unk1=32612
         AND ((id=3201 AND idx=0 AND textId=0 AND unk2=0)
           OR (id=3202 AND idx=1 AND textId IN(7100,12220) AND unk2=1)))=2;

SET @warrior_voice_1412 :=
    NOT EXISTS (SELECT 1 FROM conversation_actor WHERE entry=1412)
    AND (SELECT COUNT(*) FROM conversation_creature WHERE entry=1412)=1
    AND (SELECT COUNT(*) FROM conversation_creature WHERE entry=1412 AND id=0 AND creatureId=97167 AND creatureGuid=174984
         AND unk1=0 AND unk2=4294967289 AND duration IN(5250,6643))=1
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=1412)=1
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=1412 AND id=3214 AND idx=0 AND textId=0 AND unk1=32612 AND unk2=0)=1;

UPDATE conversation_actor SET duration=5020 WHERE entry=770 AND id=0 AND @warrior_voice_770;
UPDATE conversation_creature SET duration=8756 WHERE entry=1386 AND id=1 AND @warrior_voice_1386;
UPDATE conversation_data SET textId=4376 WHERE entry=1386 AND id=3172 AND @warrior_voice_1386;
UPDATE conversation_creature SET duration=14285 WHERE entry=1405 AND id=1 AND @warrior_voice_1405;
UPDATE conversation_data SET textId=12220 WHERE entry=1405 AND id=3202 AND @warrior_voice_1405;
UPDATE conversation_creature SET duration=6643 WHERE entry=1412 AND id=0 AND @warrior_voice_1412;

COMMIT;
