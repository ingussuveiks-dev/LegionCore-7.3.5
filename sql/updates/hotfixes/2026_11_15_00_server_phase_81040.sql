-- Server-only non-DB2 spell81040: update zone auras170 and player phases167.
-- Adapted to native735 hotfix fields/handlers, not a client-delivered hotfix.
-- Source: TrinityCore/TrinityCore#29220; both native735 effect handlers exist.
-- Reserved misc/effect IDs checked against complete native DB2 + live hotfix stores.
START TRANSACTION;
INSERT INTO spell (ID,Name,NameSubtext,Description,AuraDescription,VerifiedBuild)
SELECT 81040,'(Serverside/Non-DB2) Update Zone Auras and Phase Shift','','','',0
WHERE NOT EXISTS(SELECT 1 FROM spell WHERE ID=81040);
INSERT INTO spell_misc (ID,CastingTimeIndex,RangeIndex,SchoolMask,Attributes1,Attributes2,Attributes3,Attributes4,Attributes5,Attributes6,Attributes7,SpellID,VerifiedBuild)
SELECT 900000,1,1,1,0x29800180,0x420,0x10484005,0x10130200,0x800080,0x60008,0x1200,81040,0
WHERE NOT EXISTS(SELECT 1 FROM spell_misc WHERE ID=900000 OR SpellID=81040);
INSERT INTO spell_effect (ID,Effect,EffectIndex,EffectChainAmplitude,ImplicitTarget1,SpellID,VerifiedBuild)
SELECT 900001,170,0,1,1,81040,0
WHERE NOT EXISTS(SELECT 1 FROM spell_effect WHERE ID=900001 OR (SpellID=81040 AND DifficultyID=0 AND EffectIndex=0));
INSERT INTO spell_effect (ID,Effect,EffectIndex,EffectChainAmplitude,ImplicitTarget1,SpellID,VerifiedBuild)
SELECT 900002,167,1,1,1,81040,0
WHERE NOT EXISTS(SELECT 1 FROM spell_effect WHERE ID=900002 OR (SpellID=81040 AND DifficultyID=0 AND EffectIndex=1));
COMMIT;
