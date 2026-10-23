-- All Legion Dalaran class introductions/campaign invitations use an allowed
-- quest-status mask. Legacy74 offers the invitation only after starting it;
-- NONE1 admits the fresh/abandoned quest instead. Preserve prerequisites,
-- class/race/gender filters, custom masks and the already repaired rows.
UPDATE `spell_area` a
JOIN (
 SELECT 216443 AS spell,7502 AS area,0 AS prerequisite,41052 AS quest,1 AS classMask UNION ALL
 SELECT 190886,7502,0,38710,2 UNION ALL
 SELECT 196908,7502,0,40384,4 UNION ALL
 SELECT 201208,7502,0,40832,8 UNION ALL
 SELECT 226409,7502,0,40705,16 UNION ALL
 SELECT 200023,7502,0,40714,32 UNION ALL
 SELECT 195356,7502,0,41035,128 UNION ALL
 SELECT 204860,7502,0,40716,256 UNION ALL
 SELECT 193978,7502,0,12103,512 UNION ALL
 SELECT 199277,7502,0,40643,1024 UNION ALL
 SELECT 195286,7502,0,39047,2048 UNION ALL
 SELECT 224250,7581,40585,42597,1 UNION ALL
 SELECT 224350,7581,39756,42844,2 UNION ALL
 SELECT 224263,7581,40959,44090,4 UNION ALL
 SELECT 224286,7581,40997,43007,8 UNION ALL
 SELECT 224339,7581,41019,44100,16 UNION ALL
 SELECT 227342,7581,39799,44550,32 UNION ALL
 SELECT 227324,7581,41510,44544,64 UNION ALL
 SELECT 224338,7581,40824,44099,256 UNION ALL
 SELECT 224344,7581,40795,42186,512 UNION ALL
 SELECT 224335,7581,40653,42516,1024 UNION ALL
 SELECT 224244,7581,41069,42666,2048
) known ON a.`spell`=known.spell AND a.`area`=known.area
 AND a.`quest_start`=known.prerequisite AND a.`quest_end`=known.quest
 AND a.`classmask`=known.classMask
SET a.`quest_end_status`=1
WHERE a.`quest_end_status`=74 AND a.`autocast`=1
 AND ((known.prerequisite=0 AND a.`quest_start_status`=0)
   OR (known.prerequisite<>0 AND a.`quest_start_status`=64));

-- Native initial offer spells200023/195356 belong exclusively to DK/Mage.
-- Their templates lacked that class gate, creating foreign-class map offers.
UPDATE `quest_template_addon` SET `AllowableClasses`=32
WHERE `ID`=40714 AND `AllowableClasses`=0;
UPDATE `quest_template_addon` SET `AllowableClasses`=128
WHERE `ID`=41035 AND `AllowableClasses`=0;

-- Warrior invitation Eitrigg copied the Warlock acceptance quest ID.
UPDATE `smart_scripts` SET `event_param1`=41052
WHERE `entryorguid`=93775 AND `source_type`=0 AND `id`=2
 AND `event_type`=19 AND `event_param1`=40716 AND `action_type`=1 AND `link`=3;
