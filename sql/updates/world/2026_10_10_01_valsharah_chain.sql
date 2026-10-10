-- Val'sharah 7.3.5: native objective storage, actual objective completion,
-- both faction branches, rewards, and the Tears of Elune return journey.
-- Evidence and remaining scripted events: docs/audits/valsharah-chain-2026-10-10.md.
START TRANSACTION;

-- Two different objectives must not share counter 2. The other two quests
-- also used pre-release storage indexes; character migration remaps those.
UPDATE quest_objectives SET StorageIndex=0 WHERE ID=278367 AND QuestID=38582 AND ObjectID=92335;
UPDATE quest_objectives SET StorageIndex=0,Flags=3 WHERE ID=279313 AND QuestID=39384 AND ObjectID=95320;
UPDATE quest_objectives SET StorageIndex=3 WHERE ID=280436 AND QuestID=39384 AND ObjectID=99074;
UPDATE quest_objectives SET StorageIndex=4 WHERE ID=280885 AND QuestID=40573 AND ObjectID=99082;
UPDATE quest_objectives SET StorageIndex=5 WHERE ID=280890 AND QuestID=40573 AND ObjectID=245676;
UPDATE quest_objectives SET Flags=3 WHERE ID=280887 AND QuestID=40573 AND ObjectID=95399;
UPDATE quest_objectives SET Flags=1 WHERE ID=279312 AND QuestID=39383 AND ObjectID=95319;
UPDATE quest_objectives SET Flags2=1 WHERE ID=282868 AND QuestID=38684 AND ObjectID=136391;
INSERT INTO quest_objectives (ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,Description,VerifiedBuild)
VALUES (280418,38147,0,1,99032,1,24,0,'Open the Bramble Wall',25549)
ON DUPLICATE KEY UPDATE QuestID=38147,Type=0,StorageIndex=1,ObjectID=99032,Amount=1,Flags=24,Flags2=0;
UPDATE gameobject_template SET ScriptName='go_valsharah_bramble_wall' WHERE entry=242279 AND ScriptName IN('','go_valsharah_bramble_wall');

-- Existing spell-clicks and ordinary kills implement these objectives.
-- Event-complete shortcuts either bypassed them or blocked completion.
DELETE FROM quest_start_scripts WHERE command=7 AND id=datalong AND id IN(38142,38381,38384,38225,38235);
UPDATE quest_template SET StartScript=0 WHERE ID IN(38142,38381,38384,38225,38235) AND StartScript=ID;
UPDATE quest_template_addon SET SpecialFlags=SpecialFlags & ~2 WHERE ID IN(38142,38381,38382,38384,38225,38235,38147,39384);
DELETE FROM smart_scripts WHERE source_type=0 AND action_type=15 AND
 ((entryorguid=91045 AND action_param1=38147) OR (entryorguid=95320 AND action_param1=39384) OR
  (entryorguid=94179 AND action_param1=38382) OR (entryorguid=91109 AND action_param1=38384));
DELETE FROM creature_questender WHERE id=94179 AND quest=38142;

-- Native reward items. Deleted legacy reward spells 181865/81040 stay absent.
UPDATE quest_template SET RewardItem1=141387,RewardAmount1=1 WHERE ID=38377 AND RewardItem1=0;
UPDATE quest_template SET RewardItem1=141390,RewardAmount1=1 WHERE ID=38753 AND RewardItem1=0;
UPDATE quest_template SET QuestPackageID=0,RewardItem1=141383,RewardAmount1=1 WHERE ID=38743 AND QuestPackageID IN(0,665) AND RewardItem1 IN(0,141383);
-- 40890 keeps package 665: the four chest rewards belong to placement.
UPDATE quest_template_addon SET PrevQuestID=38743 WHERE ID=40890 AND PrevQuestID IN(40567,38743);

-- 43576 was mixed with unrelated Suramar data. TDB 25549 and native
-- QuestLineXQuest 189/190 both identify Regroup at the Refuge here.
UPDATE quest_template SET QuestSortID=7558,StartItem=0,ItemDrop1=0,ItemDropQuantity1=0,
 Flags=34603008,RewardXPDifficulty=3,RewardMoneyDifficulty=3,RewardBonusMoney=91500
 WHERE ID=43576 AND StartItem IN(0,140758);
UPDATE quest_template_addon SET ProvidedItemCount=0 WHERE ID=43576;
DELETE FROM creature_questender WHERE id=97140 AND quest=43576;

UPDATE quest_template_addon SET PrevQuestID=38381 WHERE ID IN(38235,38225);
UPDATE quest_template_addon SET PrevQuestID=38142 WHERE ID IN(38455,38922);
UPDATE quest_template_addon SET PrevQuestID=38455 WHERE ID=38143;
UPDATE quest_template_addon SET PrevQuestID=38143 WHERE ID=38144;
UPDATE quest_template_addon SET PrevQuestID=39384 WHERE ID=40573;
UPDATE quest_template_addon SET PrevQuestID=38147 WHERE ID=38148;
INSERT IGNORE INTO creature_queststarter (id,quest) VALUES (91223,38148);
UPDATE quest_template_addon SET PrevQuestID=38377 WHERE ID=38641;
UPDATE quest_template_addon SET PrevQuestID=38663 WHERE ID=38595;
UPDATE quest_template_addon SET PrevQuestID=38753 WHERE ID=41054;
UPDATE quest_template_addon SET PrevQuestID=41054 WHERE ID=41890;
UPDATE quest_template_addon SET PrevQuestID=0 WHERE ID IN(43576,38684,38377);
UPDATE quest_template_addon SET PrevQuestID=43576 WHERE ID=38675;
UPDATE quest_template_addon SET PrevQuestID=38675 WHERE ID=41749;
UPDATE quest_template_addon SET PrevQuestID=41724 WHERE ID=41893;
UPDATE quest_template_addon SET PrevQuestID=38684 WHERE ID=43702;
UPDATE quest_template_addon SET PrevQuestID=43702 WHERE ID IN(38687,41763);
UPDATE quest_template_addon SET NextQuestID=0 WHERE ID=38684 AND NextQuestID=38687;
-- Require both jobs in the barrow, not either one.
UPDATE conditions SET ElseGroup=0 WHERE SourceTypeOrReferenceId=19 AND SourceGroup=0 AND SourceEntry=38147 AND ConditionTypeOrReference=8 AND ConditionValue1 IN(38144,38145);

DELETE FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceGroup=0 AND SourceEntry IN(38143,38322,43576,38684,43702);
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionValue1,Comment) VALUES
(19,0,38143,0,8,38246,'Awaken the archdruid after destroying the totem'),
(19,0,38322,0,8,38235,'Return after both Elothir tasks'),
(19,0,43576,0,8,41708,'Regroup after Alliance vigil'),
(19,0,43576,1,8,41890,'Regroup after Horde vigil'),
(19,0,38684,0,8,41724,'Leaves after Alliance trail'),
(19,0,38684,1,8,38675,'Leaves after Horde trail'),
(19,0,43702,0,8,41893,'Nightwing after Alliance Varethos task'),
(19,0,43702,1,8,41749,'Nightwing after Horde Varethos task');

-- The old gossip required Alliance AND Horde quests simultaneously.
UPDATE conditions SET ElseGroup=2 WHERE SourceTypeOrReferenceId=15 AND SourceGroup=19419 AND SourceEntry=1 AND ConditionTypeOrReference=9 AND ConditionValue1=41763;
UPDATE conditions SET ElseGroup=3 WHERE SourceTypeOrReferenceId=15 AND SourceGroup=19419 AND SourceEntry=1 AND ConditionTypeOrReference=28 AND ConditionValue1=41763;

-- Preserve Tyrande at the completed vigil for both factions and after reward.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7558 AND SourceEntry=35;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionValue1,Comment) VALUES
(23,7558,35,0,28,41708,'Alliance vigil complete'),
(23,7558,35,1,8,41708,'Alliance vigil rewarded'),
(23,7558,35,2,28,41890,'Horde vigil complete'),
(23,7558,35,3,8,41890,'Horde vigil rewarded');

-- Quest-specific portal credit is now granted after the teleport ACK.
DELETE FROM smart_scripts WHERE entryorguid=106815 AND source_type=0 AND id=1 AND action_type=33 AND action_param1=109750;
COMMIT;
