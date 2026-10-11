param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_11_04_highmountain_audience.sql" -Raw
$sql=''
if(!$Live){
 $sql=(@('creature_template','smart_scripts','conditions') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'binding',COUNT(*) FROM creature_template WHERE entry=95256 AND AIName='' AND ScriptName='npc_audience_oakin';
SELECT 'old-shared-credit',COUNT(*) FROM smart_scripts WHERE (entryorguid=95256 AND source_type=0 AND id IN(0,1)) OR (entryorguid=9525600 AND source_type=9);
SELECT 'orphan-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=2 AND SourceEntry=95256 AND SourceId=0;
SELECT 'native-objective',COUNT(*) FROM quest_objectives WHERE ID=279991 AND QuestID=39374 AND Type=0 AND StorageIndex=0 AND ObjectID=95256 AND Amount=1 AND Flags=0 AND Flags2=0;
SELECT 'start',COUNT(*) FROM creature WHERE guid=283321 AND id=95256 AND map=1220 AND phaseId='' AND ABS(position_x-5221.07)<0.02 AND ABS(position_y-4902.25)<0.02 AND ABS(position_z-711.837)<0.02;
SELECT 'destination',COUNT(*) FROM creature WHERE guid=340589 AND id=95256 AND map=1220 AND ABS(position_x-5335.47)<0.02 AND ABS(position_y-4919.65)<0.02 AND ABS(position_z-691.969)<0.02;
SELECT 'guard',COUNT(*) FROM creature WHERE guid=283352 AND id=98904 AND map=1220 AND phaseId='' AND ABS(position_x-5338.3)<0.02 AND ABS(position_y-4922.95)<0.02 AND ABS(position_z-692.054)<0.02;
SELECT 'localized-dialogue',COUNT(*) FROM creature_text WHERE (CreatureID=95256 AND GroupID=0 AND BroadcastTextID=100809) OR (CreatureID=95256 AND GroupID=1 AND BroadcastTextID=101720) OR (CreatureID=98904 AND GroupID=0 AND BroadcastTextID=99088);
SELECT 'enders',COUNT(*) FROM creature_questender WHERE id=95256 AND quest=39374;
SELECT 'followup',COUNT(*) FROM quest_template_addon WHERE ID=39455 AND PrevQuestID=39374 AND NextQuestID=39860;
SELECT 'next-offer',COUNT(*) FROM quest_template WHERE ID=39374 AND RewardNextQuest=39455;
SELECT 'next-starter',COUNT(*) FROM creature_queststarter WHERE id=95256 AND quest=39455;
SELECT 'cave-accept',COUNT(*) FROM smart_scripts WHERE entryorguid=95256 AND source_type=0 AND id=2 AND event_type=19 AND event_param1=39455 AND action_type=80 AND action_param1=9620856;
SELECT 'cave-summon',COUNT(*) FROM smart_scripts WHERE entryorguid=9620856 AND source_type=9 AND id=0 AND action_type=12 AND action_param1=95256;
SELECT 'cave-path',COUNT(*) FROM smart_scripts WHERE entryorguid=95256 AND source_type=0 AND id=3 AND event_type=54 AND action_type=53 AND action_param2=95256;
SELECT 'cave-arrival',COUNT(*) FROM smart_scripts WHERE entryorguid=95256 AND source_type=0 AND id=4 AND event_type=40 AND event_param1=6 AND event_param2=95256 AND action_type=1 AND action_param1=3;
SELECT 'cave-waypoints',COUNT(*) FROM waypoints WHERE entry=95256;
"@
$expected=@("binding`t1","old-shared-credit`t0","orphan-conditions`t0","native-objective`t1","start`t1","destination`t1","guard`t1","localized-dialogue`t3","enders`t1","followup`t1","next-offer`t1","next-starter`t1","cave-accept`t1","cave-summon`t1","cave-path`t1","cave-arrival`t1","cave-waypoints`t7")
if(!$Live){
 $sql+=@"
SELECT 'unrelated-ai',COUNT(*) FROM saved_creature_template a JOIN creature_template b USING(entry) WHERE a.entry<>95256 AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName);
SELECT 'unrelated-smart-deletion',COUNT(*) FROM saved_smart_scripts a LEFT JOIN smart_scripts b ON a.entryorguid=b.entryorguid AND a.source_type=b.source_type AND a.id=b.id WHERE b.id IS NULL AND NOT ((a.entryorguid=95256 AND a.source_type=0 AND a.id IN(0,1)) OR (a.entryorguid=9525600 AND a.source_type=9));
SELECT 'unrelated-conditions',COUNT(*) FROM saved_conditions a LEFT JOIN conditions b ON a.SourceTypeOrReferenceId=b.SourceTypeOrReferenceId AND a.SourceGroup=b.SourceGroup AND a.SourceEntry=b.SourceEntry AND a.SourceId=b.SourceId AND a.ElseGroup=b.ElseGroup AND a.ConditionTypeOrReference=b.ConditionTypeOrReference AND a.ConditionTarget=b.ConditionTarget AND a.ConditionValue1=b.ConditionValue1 AND a.ConditionValue2=b.ConditionValue2 AND a.ConditionValue3=b.ConditionValue3 WHERE b.SourceTypeOrReferenceId IS NULL AND NOT(a.SourceTypeOrReferenceId=22 AND a.SourceGroup=2 AND a.SourceEntry=95256 AND a.SourceId=0);
"@
 $expected+=@("unrelated-ai`t0","unrelated-smart-deletion`t0","unrelated-conditions`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Audience SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Audience SQL mismatch: $($actual -join '|')"}
'PASS: repeated migration, native objective, removed shared credit, real endpoint/guard, localized dialogue and preserved follow-up acceptance/summon/path.'
