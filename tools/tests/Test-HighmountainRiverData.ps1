param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_10_06_highmountain_river.sql" -Raw
$native=Get-Content "$repo/docs/audits/highmountain-river-native-2026-10-10.json" -Raw | ConvertFrom-Json
$nativeRows=($native.objectives | ForEach-Object {"SELECT $($_.ID) ID,$($_.QuestID) QuestID,$($_.Type) Type,$($_.StorageIndex) StorageIndex,$($_.ObjectID) ObjectID,$($_.Amount) Amount,$($_.Flags) Flags,$($_.Flags2) Flags2"}) -join ' UNION ALL '
$sql=''
if(!$Live){
 $sql=(@('creature_template','smart_scripts','conditions','quest_template_addon') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'native',COUNT(*) FROM ($nativeRows) a LEFT JOIN quest_objectives b USING(ID) WHERE b.ID IS NULL OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2;
SELECT 'carp',COUNT(*) FROM creature_template WHERE entry=95148 AND AIName='' AND ScriptName='npc_highmountain_whitewater_carp';
SELECT 'old-timer',COUNT(*) FROM smart_scripts WHERE (entryorguid=95148 AND source_type=0 AND event_type=8 AND event_param1=188447) OR (entryorguid=9514800 AND source_type=9);
SELECT 'wrong-water',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=9514800 AND SourceId=9;
SELECT 'native-kick',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=95148 AND spell_id=188447 AND cast_flags=1;
SELECT 'six-grub-casts',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid IN(95013,96124) AND event_type=61 AND action_type=85 AND action_param1=190421 AND action_param2=2 AND target_type=1;
SELECT 'grub-attack',COUNT(*) FROM smart_scripts WHERE entryorguid=94688 AND source_type=0 AND id=0 AND event_type=54 AND action_type=49 AND target_type=129;
SELECT 'spray-credit',COUNT(*) FROM smart_scripts WHERE entryorguid IN(95013,96124) AND source_type=0 AND event_type=8 AND event_param1=188465 AND action_type=33 AND action_param1=95017 AND target_type=7 AND link<>0;
SELECT 'spray-guards',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceId=0 AND ((SourceEntry=95013 AND SourceGroup=1) OR (SourceEntry=96124 AND SourceGroup=2)) AND ConditionTarget=0 AND ((ConditionTypeOrReference=9 AND ConditionValue1=39277) OR ConditionTypeOrReference=36);
SELECT 'provided-item',COUNT(*) FROM quest_template q JOIN quest_template_addon a USING(ID) WHERE q.ID=39277 AND q.StartItem=127988 AND a.ProvidedItemCount=1 AND q.QuestPackageID=18473;
SELECT 'parallel',COUNT(*) FROM quest_template_addon WHERE ID IN(39316,39277,39614) AND PrevQuestID=39496;
SELECT 'no-serial',COUNT(*) FROM quest_template_addon WHERE ID IN(39316,39614) AND NextQuestID<>0;
SELECT 'lifespring',COUNT(*) FROM quest_template_addon WHERE ID=39661 AND PrevQuestID=39277;
SELECT 'barricade',COUNT(*) FROM gameobject_template t JOIN gameobject g ON g.id=t.entry WHERE t.entry=243368 AND t.type=10 AND g.map=1220;
SELECT 'gate-response',COUNT(*) FROM smart_scripts WHERE entryorguid=243368 AND source_type=1 AND event_type=70 AND event_param1=2 AND action_type=45 AND target_param1=95080;
SELECT 'starters',COUNT(*) FROM creature_queststarter WHERE (id=95186 AND quest IN(39316,39614)) OR (id=95956 AND quest IN(39277,39661));
SELECT 'enders',COUNT(*) FROM creature_questender WHERE (id=95186 AND quest IN(39316,39614)) OR (id=95956 AND quest=39277) OR (id=96520 AND quest=39661);
SELECT 'available-spawns',(SELECT COUNT(*)>=8 FROM creature WHERE id=95148 AND map=1220),(SELECT COUNT(*)>=6 FROM creature WHERE id IN(95013,96124) AND map=1220);
"@
$expected=@("native`t0","carp`t1","old-timer`t0","wrong-water`t0","native-kick`t1","six-grub-casts`t6","grub-attack`t1","spray-credit`t2","spray-guards`t4","provided-item`t1","parallel`t3","no-serial`t0","lifespring`t1","barricade`t1","gate-response`t1","starters`t4","enders`t4","available-spawns`t1`t1")
if(!$Live){
 $sql+=@"
SELECT 'unrelated-smart',COUNT(*) FROM smart_scripts a WHERE a.entryorguid NOT IN(95148,9514800,95013,96124,94688) AND NOT EXISTS(SELECT 1 FROM saved_smart_scripts b WHERE a.entryorguid=b.entryorguid AND a.source_type=b.source_type AND a.id=b.id AND a.action_type=b.action_type AND a.action_param1=b.action_param1 AND a.target_type=b.target_type AND a.link=b.link);
SELECT 'grub-cleanup',COUNT(*) FROM smart_scripts WHERE entryorguid=94688 AND source_type=0 AND id=1 AND event_type=38 AND action_type=41;
UPDATE creature_template SET ScriptName='custom_carp',AIName='custom_ai' WHERE entry=95148;
$migration
SELECT 'custom',ScriptName,AIName FROM creature_template WHERE entry=95148;
"@
 $expected+=@("unrelated-smart`t0","grub-cleanup`t1","custom`tcustom_carp`tcustom_ai")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'River SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($rows -join '|') -ne ($expected -join '|')){throw "River SQL mismatch: $($rows -join '|')"}
'PASS: native objectives, real rescue binding, targeted allied grubs, quest guards, repeatable migration, parallel offers, item/reward, barricade and Lifespring handoff.'
