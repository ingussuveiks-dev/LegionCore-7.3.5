-- Functional reconstruction: original server-only179109 payload not recovered.
-- Actual guarded nest-defence behavior is supplied by spell_arcane_infused_egg.
-- No client hotfix delivery, direct item grant or quest credit.
START TRANSACTION;
INSERT INTO spell (ID,Name,NameSubtext,Description,AuraDescription,VerifiedBuild)
SELECT 179109,'(Serverside/Reconstructed) Arcane-infused Egg Nest Defence','','','',0
WHERE NOT EXISTS(SELECT 1 FROM spell WHERE ID=179109);
INSERT INTO spell_misc (ID,CastingTimeIndex,RangeIndex,SchoolMask,Attributes1,Attributes2,Attributes3,Attributes4,Attributes5,Attributes6,Attributes7,SpellID,VerifiedBuild)
SELECT 900010,1,1,1,0x29800180,0x420,0x10484005,0x10130200,0x800080,0x60008,0x1200,179109,0
WHERE NOT EXISTS(SELECT 1 FROM spell_misc WHERE ID=900010 OR SpellID=179109);
INSERT INTO spell_effect (ID,Effect,EffectIndex,EffectChainAmplitude,ImplicitTarget1,SpellID,VerifiedBuild)
SELECT 900011,3,0,1,1,179109,0
WHERE NOT EXISTS(SELECT 1 FROM spell_effect WHERE ID=900011 OR (SpellID=179109 AND DifficultyID=0 AND EffectIndex=0));
COMMIT;
