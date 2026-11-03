-- Matching26972 Garona2432 full enUS voices and native3000/1000ms gaps.
-- Preserve exact actor roles/packed fields, custom definitions and5000ms padding.
START TRANSACTION;
SET @garona_outro_voice :=
 NOT EXISTS(SELECT 1 FROM conversation_actor WHERE entry=2432)
 AND (SELECT COUNT(*) FROM conversation_creature WHERE entry=2432)=2
 AND (SELECT COUNT(*) FROM conversation_creature WHERE entry=2432 AND creatureId=108214 AND creatureGuid=4651523 AND unk1=0 AND unk2=0
      AND ((id=0 AND duration=2350) OR (id=1 AND duration IN(2350,11814))))=2
 AND (SELECT COUNT(*) FROM conversation_data WHERE entry=2432)=2
 AND (SELECT COUNT(*) FROM conversation_data WHERE entry=2432 AND unk1=95
      AND ((id=5151 AND idx=0 AND textId=0 AND unk2=0) OR (id=5152 AND idx=1 AND textId IN(11800,10693) AND unk2=538968064)))=2;
UPDATE conversation_creature SET duration=11814 WHERE entry=2432 AND id=1 AND @garona_outro_voice;
UPDATE conversation_data SET textId=10693 WHERE entry=2432 AND id=5152 AND @garona_outro_voice;
COMMIT;
