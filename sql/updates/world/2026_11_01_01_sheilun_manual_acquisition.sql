-- PENDING: keep outside the live updater until the matching executable is
-- staged, verified and promoted offline. Native data remain authoritative;
-- the derived pool/foot placement and flight connector are documented in the
-- acquisition audit, rather than presented as recovered retail coordinates.
START TRANSACTION;
CREATE TEMPORARY TABLE sheilun_acquisition_admission (ok TINYINT NOT NULL CHECK (ok=1));
INSERT INTO sheilun_acquisition_admission
SELECT
 (SELECT COUNT(*) FROM creature_template WHERE
   (entry=102479 AND AIName='' AND ScriptName IN ('','npc_sheilun_owned_staff') AND npcflag=16777216) OR
   (entry=102648 AND AIName='' AND ScriptName IN ('','npc_sheilun_return_kite') AND VehicleId=4581) OR
   (entry=101880 AND AIName='SmartAI' AND ScriptName IN ('','npc_sheilun_return_guide') AND gossip_menu_id=19254))=3
 AND EXISTS (SELECT 1 FROM instance_template WHERE map=1541 AND script='instance_sheylun')
 AND EXISTS (SELECT 1 FROM scenario_data WHERE ScenarioID=1007 AND MapID=1541 AND DifficultyID IN(8,12) AND Team=0 AND Class=10 AND LfgDungeonID=0)
 AND EXISTS (SELECT 1 FROM spell_target_position WHERE id=202742 AND target_map=1541 AND
   ABS(target_position_x+1073)<0.0001 AND ABS(target_position_y+3143.71)<0.001 AND
   ABS(target_position_z-27.66)<0.0001 AND ABS(target_orientation-0.02)<0.0001)
 AND NOT EXISTS (SELECT 1 FROM lfg_entrances WHERE dungeonId=1168 AND NOT
   (ABS(position_x+1073)<0.0001 AND ABS(position_y+3143.71)<0.001 AND
    ABS(position_z-27.66)<0.0001 AND ABS(orientation-0.02)<0.0001))
 AND EXISTS (SELECT 1 FROM gameobject_template WHERE entry=256913 AND type=3 AND Data1 IN (0,256913) AND Data3=0 AND ScriptName='go_art_sheylun')
 AND (SELECT COUNT(*) FROM gameobject_loot_template WHERE Entry=256913)=1
 AND EXISTS (SELECT 1 FROM gameobject_loot_template WHERE Entry=256913 AND Item=128937 AND Currency=0 AND Reference=0 AND Chance=100 AND
     QuestRequired=0 AND LootMode=1 AND GroupId=0 AND MinCount=1 AND MaxCount=1)
 AND EXISTS (SELECT 1 FROM npc_spellclick_spells WHERE npc_entry=102479 AND spell_id=202435 AND cast_flags=1)
 AND EXISTS (SELECT 1 FROM npc_spellclick_spells WHERE npc_entry=102648 AND spell_id=46598 AND cast_flags=1)
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=101880 AND source_type=0)=12
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=101880 AND source_type=0 AND event_type=62)=7
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=101880 AND source_type=0 AND event_type=61)=5
 AND NOT EXISTS (SELECT 1 FROM creature_text WHERE (CreatureID=101881 AND GroupID BETWEEN 13 AND 18 OR CreatureID=101880 AND GroupID=13) AND NOT
   (ID=0 AND Type=12 AND Language=0 AND Probability=100 AND Emote=0 AND Duration=0 AND Sound=0 AND MinTimer=0 AND MaxTimer=0 AND SpellID=0 AND
    ((CreatureID=101881 AND GroupID=13 AND BroadcastTextID=104773 AND Text='It is done! You have done a great service for Pandaria this day.') OR
     (CreatureID=101881 AND GroupID=14 AND BroadcastTextID=104774 AND Text='There it is, the staff of our last emperor. You have proven yourself worthy to wield it, friend of the Shado-Pan.') OR
     (CreatureID=101881 AND GroupID=15 AND BroadcastTextID=104775 AND Text='Ten thousand years ago, Emperor Shaohao used that very staff to cloak our land in a veil of mists, hiding us from the Burning Legion.') OR
     (CreatureID=101881 AND GroupID=16 AND BroadcastTextID=104776 AND Text='But this time, the Pandaren will not hide away. We chose to FIGHT!') OR
     (CreatureID=101881 AND GroupID=17 AND BroadcastTextID=104777 AND Text='Turn the emperor''s staff against the Legion, champion. We will be right behind you.') OR
     (CreatureID=101881 AND GroupID=18 AND BroadcastTextID=104785 AND Text='Thank you, my friend! May the White Tiger bless your campaign against our foes!') OR
     (CreatureID=101880 AND GroupID=13 AND BroadcastTextID=104958 AND Text='Back to turtle! Dis place too wet anyways.'))))
 AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id IN (202435,202437,202828,202831) AND
   ScriptName NOT IN ('spell_sheilun_manual_claim','spell_sheilun_claim_scene','spell_sheilun_return_boarding','spell_sheilun_return_transfer'))
 AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE
   ScriptName IN ('spell_sheilun_manual_claim','spell_sheilun_claim_scene','spell_sheilun_return_boarding','spell_sheilun_return_transfer') AND
   NOT ((spell_id=202435 AND ScriptName='spell_sheilun_manual_claim') OR
        (spell_id=202437 AND ScriptName='spell_sheilun_claim_scene') OR
        (spell_id=202828 AND ScriptName='spell_sheilun_return_boarding') OR
        (spell_id=202831 AND ScriptName='spell_sheilun_return_transfer')));
DROP TEMPORARY TABLE sheilun_acquisition_admission;

-- Native26972 LFG1168 is map1541/NormalScenario12, with no direct ScenarioID.
-- Its class route used obsolete8, and its missing entrance made the strict
-- native resume validator reject both live rejoin and fresh entrance restart.
-- Reuse the actual native outbound202742 destination; no later checkpoint,
-- item, quest counter or scenario criterion is synthesized by this repair.
UPDATE scenario_data SET DifficultyID=12 WHERE ScenarioID=1007 AND MapID=1541;
INSERT IGNORE INTO lfg_entrances (dungeonId,position_x,position_y,position_z,orientation)
SELECT 1168,target_position_x,target_position_y,target_position_z,target_orientation
FROM spell_target_position WHERE id=202742;

UPDATE creature_template SET ScriptName='npc_sheilun_owned_staff' WHERE entry=102479;
UPDATE creature_template SET ScriptName='npc_sheilun_return_kite' WHERE entry=102648;
-- The scoped delegate inherits SmartAI for every shared outgoing19254 offer.
UPDATE creature_template SET ScriptName='npc_sheilun_return_guide' WHERE entry=101880;
UPDATE gameobject_template SET Data1=256913 WHERE entry=256913;
INSERT IGNORE INTO spell_script_names (spell_id,ScriptName) VALUES
 (202435,'spell_sheilun_manual_claim'), (202437,'spell_sheilun_claim_scene'),
 (202828,'spell_sheilun_return_boarding'), (202831,'spell_sheilun_return_transfer');

INSERT INTO creature_text
 (CreatureID,GroupID,ID,Text,Type,Language,Probability,Emote,Duration,Sound,BroadcastTextID,MinTimer,MaxTimer,SpellID)
VALUES
 (101881,13,0,'It is done! You have done a great service for Pandaria this day.',12,0,100,0,0,0,104773,0,0,0),
 (101881,14,0,'There it is, the staff of our last emperor. You have proven yourself worthy to wield it, friend of the Shado-Pan.',12,0,100,0,0,0,104774,0,0,0),
 (101881,15,0,'Ten thousand years ago, Emperor Shaohao used that very staff to cloak our land in a veil of mists, hiding us from the Burning Legion.',12,0,100,0,0,0,104775,0,0,0),
 (101881,16,0,'But this time, the Pandaren will not hide away. We chose to FIGHT!',12,0,100,0,0,0,104776,0,0,0),
 (101881,17,0,'Turn the emperor''s staff against the Legion, champion. We will be right behind you.',12,0,100,0,0,0,104777,0,0,0),
 (101881,18,0,'Thank you, my friend! May the White Tiger bless your campaign against our foes!',12,0,100,0,0,0,104785,0,0,0),
 (101880,13,0,'Back to turtle! Dis place too wet anyways.',12,0,100,0,0,0,104958,0,0,0)
ON DUPLICATE KEY UPDATE Text=VALUES(Text),Type=VALUES(Type),Language=VALUES(Language),Probability=VALUES(Probability),
 Emote=VALUES(Emote),Duration=VALUES(Duration),Sound=VALUES(Sound),BroadcastTextID=VALUES(BroadcastTextID),
 MinTimer=VALUES(MinTimer),MaxTimer=VALUES(MaxTimer),SpellID=VALUES(SpellID);
COMMIT;
