-- Matching26972 enUS full voices and ConversationLine native6000/1000ms gaps.
-- 3999:5974+6000 ->4018:2183+1000; content15157, native padding5000.
-- Preserve original virtual actors/packed fields and custom definitions.
START TRANSACTION;
SET @zirus_death_voice :=
 NOT EXISTS(SELECT 1 FROM conversation_creature WHERE entry=1890)
 AND (SELECT COUNT(*) FROM conversation_actor WHERE entry=1890)=2
 AND (SELECT COUNT(*) FROM conversation_actor WHERE entry=1890 AND unk1=0 AND unk2=1 AND unk3=0
      AND ((id=0 AND actorId=52523 AND creatureId=105542 AND displayId=69190 AND duration=2600)
        OR (id=1 AND actorId=52490 AND creatureId=105464 AND displayId=69095 AND duration IN(2600,12557))))=2
 AND (SELECT COUNT(*) FROM conversation_data WHERE entry=1890)=2
 AND (SELECT COUNT(*) FROM conversation_data WHERE entry=1890
      AND ((id=3999 AND idx=0 AND textId=0 AND unk1=627 AND unk2=0)
        OR (id=4018 AND idx=1 AND textId IN(8600,11974) AND unk1=107 AND unk2=1)))=2;
UPDATE conversation_actor SET duration=12557 WHERE entry=1890 AND id=1 AND @zirus_death_voice;
UPDATE conversation_data SET textId=11974 WHERE entry=1890 AND id=4018 AND @zirus_death_voice;
COMMIT;
