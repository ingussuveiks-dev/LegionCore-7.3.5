-- Matching26972 enUS opening voices plus authored ConversationLine gaps.
-- Preserve original actors, IDs, packed flags and native5000ms padding.
-- Each definition accepts only exact original/canonical partial cells; custom rows are untouched.
START TRANSACTION;
SET @kingslayers_voice_2361 :=
 NOT EXISTS(SELECT 1 FROM conversation_actor WHERE entry=2361)
 AND (SELECT COUNT(*) FROM conversation_creature WHERE entry=2361)=6
 AND (SELECT COUNT(*) FROM conversation_creature WHERE entry=2361 AND ((id=0 AND creatureId=108015 AND creatureGuid=0 AND unk1=0 AND unk2=0 AND duration=1950)
      OR (id=1 AND creatureId=98094 AND creatureGuid=4650992 AND unk1=0 AND unk2=0 AND duration=1950)
      OR (id=2 AND creatureId=108015 AND creatureGuid=0 AND unk1=0 AND unk2=0 AND duration=1950)
      OR (id=3 AND creatureId=98094 AND creatureGuid=4650992 AND unk1=0 AND unk2=0 AND duration=1950)
      OR (id=4 AND creatureId=98094 AND creatureGuid=4650992 AND unk1=0 AND unk2=0 AND duration=1950)
      OR (id=5 AND creatureId=98094 AND creatureGuid=4650992 AND unk1=0 AND unk2=0 AND duration IN(1950,26090))))=6
 AND (SELECT COUNT(*) FROM conversation_data WHERE entry=2361)=6
 AND (SELECT COUNT(*) FROM conversation_data WHERE entry=2361 AND ((id=5016 AND idx=0 AND textId=0 AND unk1=4229220032 AND unk2=0)
      OR (id=5007 AND idx=1 AND textId IN(4650,5874) AND unk1=4229220032 AND unk2=1)
      OR (id=5013 AND idx=2 AND textId IN(9550,11511) AND unk1=4229220032 AND unk2=0)
      OR (id=5017 AND idx=3 AND textId IN(16450,18863) AND unk1=4229220032 AND unk2=1)
      OR (id=5018 AND idx=4 AND textId IN(23300,25351) AND unk1=4229220032 AND unk2=1)
      OR (id=5021 AND idx=5 AND textId IN(28500,32864) AND unk1=4229220032 AND unk2=1)))=6;
UPDATE conversation_creature SET duration=26090 WHERE entry=2361 AND id=5 AND @kingslayers_voice_2361;
UPDATE conversation_data SET textId=5874 WHERE entry=2361 AND id=5007 AND @kingslayers_voice_2361;
UPDATE conversation_data SET textId=11511 WHERE entry=2361 AND id=5013 AND @kingslayers_voice_2361;
UPDATE conversation_data SET textId=18863 WHERE entry=2361 AND id=5017 AND @kingslayers_voice_2361;
UPDATE conversation_data SET textId=25351 WHERE entry=2361 AND id=5018 AND @kingslayers_voice_2361;
UPDATE conversation_data SET textId=32864 WHERE entry=2361 AND id=5021 AND @kingslayers_voice_2361;
SET @kingslayers_voice_2377 :=
 NOT EXISTS(SELECT 1 FROM conversation_actor WHERE entry=2377)
 AND (SELECT COUNT(*) FROM conversation_creature WHERE entry=2377)=4
 AND (SELECT COUNT(*) FROM conversation_creature WHERE entry=2377 AND ((id=0 AND creatureId=108015 AND creatureGuid=4651076 AND unk1=0 AND unk2=0 AND duration=3650)
      OR (id=1 AND creatureId=108015 AND creatureGuid=4651076 AND unk1=0 AND unk2=0 AND duration=3650)
      OR (id=2 AND creatureId=108015 AND creatureGuid=4651076 AND unk1=0 AND unk2=0 AND duration=3650)
      OR (id=3 AND creatureId=108015 AND creatureGuid=4651076 AND unk1=0 AND unk2=0 AND duration IN(3650,31729))))=4
 AND (SELECT COUNT(*) FROM conversation_data WHERE entry=2377)=4
 AND (SELECT COUNT(*) FROM conversation_data WHERE entry=2377 AND ((id=5037 AND idx=0 AND textId=0 AND unk1=4229237488 AND unk2=0)
      OR (id=5040 AND idx=1 AND textId IN(7250,12307) AND unk1=4229237488 AND unk2=0)
      OR (id=5048 AND idx=2 AND textId IN(17100,24696) AND unk1=4229237488 AND unk2=0)
      OR (id=5962 AND idx=3 AND textId IN(24250,36110) AND unk1=4229237488 AND unk2=0)))=4;
UPDATE conversation_creature SET duration=31729 WHERE entry=2377 AND id=3 AND @kingslayers_voice_2377;
UPDATE conversation_data SET textId=12307 WHERE entry=2377 AND id=5040 AND @kingslayers_voice_2377;
UPDATE conversation_data SET textId=24696 WHERE entry=2377 AND id=5048 AND @kingslayers_voice_2377;
UPDATE conversation_data SET textId=36110 WHERE entry=2377 AND id=5962 AND @kingslayers_voice_2377;
COMMIT;
