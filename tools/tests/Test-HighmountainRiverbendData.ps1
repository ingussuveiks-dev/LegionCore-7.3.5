param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_11_00_highmountain_riverbend.sql" -Raw
$native=Get-Content "$repo/docs/audits/highmountain-riverbend-native-2026-10-11.json" -Raw | ConvertFrom-Json
$rows=($native.objectives | ForEach-Object {"SELECT $($_.ID) ID,$($_.QuestID) QuestID,$($_.Type) Type,$($_.StorageIndex) StorageIndex,$($_.ObjectID) ObjectID,$($_.Amount) Amount,$($_.Flags) Flags,$($_.Flags2) Flags2"}) -join ' UNION ALL '
$sql=''
if(!$Live){
 $sql=(@('creature_template','smart_scripts','quest_objectives') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'native-objectives',COUNT(*) FROM ($rows) a LEFT JOIN quest_objectives b USING(ID) WHERE b.ID IS NULL OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2;
SELECT 'evac-ai',COUNT(*) FROM creature_template WHERE entry=100520 AND AIName='' AND ScriptName='npc_riverbend_evacuee';
SELECT 'old-horn',COUNT(*) FROM smart_scripts WHERE entryorguid=100520 AND source_type=0 AND id IN(0,1,2) AND event_type=8 AND event_param1=210891;
SELECT 'no-reseat',COUNT(*) FROM smart_scripts WHERE entryorguid IN(94255,95051,94286) AND source_type=0 AND id=4 AND event_type=60 AND action_type=11 AND action_param1=46598;
SELECT 'grasp-binding',COUNT(*) FROM creature_template WHERE entry=97648 AND ScriptName='npc_grasp_of_underking_quest' AND VehicleId=4397 AND npcflag&1;
SELECT 'horn-button',COUNT(*) FROM spell_area WHERE spell=210983 AND area=7709 AND quest_start=42104 AND quest_start_status=8 AND quest_end=0 AND autocast=1;
SELECT 'evac-spawns',COUNT(*) FROM creature WHERE id=100520 AND map=1220 AND areaId=7709;
SELECT 'grasp-spawns',COUNT(*) FROM creature WHERE guid IN(340592,340593,340595) AND id=97648 AND PhaseId='5645';
SELECT 'raider-sources',(SELECT COUNT(*) FROM creature WHERE id=102274 AND map=1220),(SELECT COUNT(*) FROM creature WHERE id=103067 AND map=1220),(SELECT KillCredit2 FROM creature_template_wdb WHERE Entry=103067);
SELECT 'cave-spawns',(SELECT COUNT(*)>=3 FROM creature WHERE id=94261 AND map=1220),(SELECT COUNT(*)>=3 FROM creature WHERE id=98653 AND map=1220);
SELECT 'scaling',COUNT(*) FROM creature_template_scaling WHERE Entry IN(102274,103067,94261,98653) AND LevelScalingMin=98 AND LevelScalingMax=110;
SELECT 'starters',COUNT(*) FROM creature_queststarter WHERE (id=97662 AND quest=42104) OR (id=93805 AND quest IN(39025,39026)) OR (id=97553 AND quest=39043);
SELECT 'enders',COUNT(*) FROM creature_questender WHERE (id=93805 AND quest=42104) OR (id=97553 AND quest IN(39025,39026,39043));
SELECT 'chain',COUNT(*) FROM quest_template_addon WHERE (ID=42104 AND PrevQuestID=39498) OR (ID IN(39025,39026) AND PrevQuestID=42104) OR (ID=39043 AND PrevQuestID=39026 AND NextQuestID=39027);
SELECT 'scene',COUNT(*) FROM smart_scripts WHERE entryorguid=93805 AND source_type=0 AND event_type=20 AND event_param1=42104 AND action_type=85 AND action_param1=192268;
SELECT 'phase',COUNT(*) FROM phase_definitions WHERE zoneId=7503 AND entry=9 AND phaseId='5645';
SELECT 'phase-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7503 AND SourceEntry IN(8,9,10,11);
"@
$expected=@("native-objectives`t0","evac-ai`t1","old-horn`t0","no-reseat`t0","grasp-binding`t1","horn-button`t1","evac-spawns`t17","grasp-spawns`t3","raider-sources`t10`t2`t102274","cave-spawns`t1`t1","scaling`t4","starters`t4","enders`t4","chain`t4","scene`t1","phase`t1","phase-conditions`t0")
if(!$Live){
 $sql+=@"
SELECT 'unrelated-objectives',COUNT(*) FROM saved_quest_objectives a LEFT JOIN quest_objectives b USING(ID) WHERE a.ID NOT IN(283671,283672) AND (b.ID IS NULL OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2);
SELECT 'unrelated-ai',COUNT(*) FROM saved_creature_template a JOIN creature_template b USING(entry) WHERE a.entry<>100520 AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName);
SELECT 'unrelated-smart',COUNT(*) FROM saved_smart_scripts a LEFT JOIN smart_scripts b ON a.entryorguid=b.entryorguid AND a.source_type=b.source_type AND a.id=b.id WHERE b.id IS NULL AND NOT ((a.entryorguid=100520 AND a.source_type=0 AND a.id IN(0,1,2) AND a.event_type=8 AND a.event_param1=210891) OR (a.entryorguid IN(94255,95051,94286) AND a.source_type=0 AND a.id=4 AND a.event_type=60 AND a.action_type=11 AND a.action_param1=46598 AND a.target_type=11 AND a.target_param1=97648));
"@
 $expected+=@("unrelated-objectives`t0","unrelated-ai`t0","unrelated-smart`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Riverbend SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Riverbend SQL mismatch: $($actual -join '|')"}
'PASS: native objectives, repeatable migration, horn/vehicle/scene bindings, available targets and NPCs, retained prerequisites/phases and unrelated data.'
