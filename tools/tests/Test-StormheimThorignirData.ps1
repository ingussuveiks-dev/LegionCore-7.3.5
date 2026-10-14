param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_14_12_stormheim_thorignir_handoff.sql" -Raw
$sql=''
if(!$Live){
 $sql="CREATE TEMPORARY TABLE saved_quest_template_addon AS SELECT * FROM quest_template_addon WHERE ID IN(38611,38473,39597,38459,38312,38612); CREATE TEMPORARY TABLE quest_template_addon AS SELECT * FROM saved_quest_template_addon; UPDATE quest_template_addon SET PrevQuestID=38459,NextQuestID=38612 WHERE ID=38611;`n$migration`n$migration`n"
}
$sql+=@'
SELECT 'both-blessing-gates',COUNT(*) FROM quest_template_addon WHERE ID IN(38473,38611) AND PrevQuestID=39597;
SELECT 'native-next-links',COUNT(*) FROM quest_template WHERE (ID=39597 AND RewardNextQuest=38473) OR(ID=38473 AND RewardNextQuest=38312) OR(ID=38611 AND RewardNextQuest=38612);
SELECT 'both-empty-breadcrumbs',COUNT(*) FROM quest_objectives WHERE QuestID IN(38473,38611);
SELECT 'normal-breadcrumb-completion',COUNT(*) FROM quest_template WHERE ID IN(38473,38611) AND QuestType=2 AND StartScript=0 AND CompleteScript=0 AND RewardSpell=0;
SELECT 'preserved-race-masks',COUNT(*) FROM quest_template WHERE (ID=38473 AND AllowableRaces=824181837) OR(ID=38611 AND AllowableRaces=234881970);
SELECT 'havi-offers-both',COUNT(*) FROM creature_queststarter WHERE id=96254 AND quest IN(38473,38611);
SELECT 'correct-receivers',COUNT(*) FROM creature_questender WHERE (id=91222 AND quest=38473) OR(id=92561 AND quest=38611);
SELECT 'next-offers',COUNT(*) FROM creature_queststarter WHERE (id=91222 AND quest=38312) OR(id=92561 AND quest=38612);
SELECT 'next-gates',COUNT(*) FROM quest_template_addon WHERE (ID=38312 AND PrevQuestID=38473) OR(ID=38612 AND PrevQuestID=38611);
SELECT 'source-anchors',COUNT(*) FROM creature c JOIN creature_template t ON c.id=t.entry WHERE c.map=1220 AND (t.npcflag&2)<>0 AND ((c.guid=340423 AND c.id=96254 AND c.PhaseId='') OR(c.guid=294395 AND c.id=91222 AND c.PhaseId='') OR(c.guid=290793 AND c.id=92561 AND c.PhaseId=''));
SELECT 'alliance-next-phase',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=17 AND ConditionTypeOrReference=8 AND ConditionValue1=38473;
SELECT 'no-horde-skip-source',COUNT(*) FROM quest_template_addon WHERE ID=38459 AND NextQuestID=38331;
'@
$expected=@("both-blessing-gates`t2","native-next-links`t3","both-empty-breadcrumbs`t0","normal-breadcrumb-completion`t2","preserved-race-masks`t2","havi-offers-both`t2","correct-receivers`t2","next-offers`t2","next-gates`t2","source-anchors`t3","alliance-next-phase`t1","no-horde-skip-source`t1")
if(!$Live){
 $sql+=@"
SELECT 'unrelated-addons-preserved',COUNT(*) FROM quest_template_addon a JOIN saved_quest_template_addon b USING(ID) WHERE a.ID<>38611 AND (a.PrevQuestID<>b.PrevQuestID OR a.NextQuestID<>b.NextQuestID OR a.SpecialFlags<>b.SpecialFlags);
SELECT 'horde-other-fields-preserved',COUNT(*) FROM quest_template_addon a JOIN saved_quest_template_addon b USING(ID) WHERE a.ID=38611 AND (a.NextQuestID<>b.NextQuestID OR a.SpecialFlags<>b.SpecialFlags OR a.ExclusiveGroup<>b.ExclusiveGroup);
UPDATE quest_template_addon SET PrevQuestID=12345 WHERE ID=38611;
$migration
SELECT 'custom-predecessor',COUNT(*) FROM quest_template_addon WHERE ID=38611 AND PrevQuestID=12345;
UPDATE quest_template_addon SET PrevQuestID=38459,NextQuestID=999 WHERE ID=38611;
$migration
SELECT 'custom-next-link',COUNT(*) FROM quest_template_addon WHERE ID=38611 AND PrevQuestID=38459 AND NextQuestID=999;
"@
 $expected+=@("unrelated-addons-preserved`t0","horde-other-fields-preserved`t0","custom-predecessor`t1","custom-next-link`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Thorignir SQL failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Thorignir SQL mismatch: $($actual -join '|')"}
'PASS: migration twice, both blessing prerequisites/native empty breadcrumbs/race masks/source and next relationships, physical questgivers/Alliance reward phase, no old early offer and custom/unrelated preservation.'
