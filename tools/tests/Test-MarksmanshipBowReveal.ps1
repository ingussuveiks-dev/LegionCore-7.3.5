param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-bow-reveal"
$common="$repo/build-extractors/runtime-test/marksmanship-herald-challenge"
$search="$repo/build-extractors/runtime-test/marksmanship-search-path"
New-Item -ItemType Directory -Force $out | Out-Null
& "$PSScriptRoot/Test-MarksmanshipSearchPath.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary *> "$out/search-prerequisite.log"
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_12_marksmanship_bow_reveal.sql" -Raw
$scope='SourceTypeOrReferenceId=22 AND SourceEntry=100397 AND SourceGroup IN(46,51)'
$reset="DELETE FROM conditions WHERE $scope; DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart; DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actor; DELETE FROM gameobject_template; INSERT INTO gameobject_template SELECT * FROM saved_object; DELETE FROM gameobject; INSERT INTO gameobject SELECT * FROM saved_spawn; DELETE FROM waypoints; INSERT INTO waypoints SELECT * FROM saved_path;"
$sql=@"
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=100397;
SET @known_reveal=(SELECT COUNT(*) FROM saved_smart WHERE source_type=0 AND ((id=45 AND link=50) OR (id=49 AND link=51) OR (id=50 AND link=0)))=3;
UPDATE saved_smart SET link=CASE id WHEN 45 THEN 0 WHEN 49 THEN 50 WHEN 50 THEN 51 END WHERE source_type=0 AND id IN(45,49,50) AND @known_reveal;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart; INSERT INTO smart_scripts SELECT * FROM saved_smart;
CREATE TEMPORARY TABLE expected_smart LIKE saved_smart; INSERT INTO expected_smart SELECT * FROM saved_smart;
UPDATE expected_smart SET link=CASE id WHEN 45 THEN 50 WHEN 49 THEN 51 WHEN 50 THEN 0 END WHERE source_type=0 AND id IN(45,49,50);
CREATE TEMPORARY TABLE saved_actor LIKE creature_template; INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=100397;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor; INSERT INTO creature_template SELECT * FROM saved_actor;
CREATE TEMPORARY TABLE saved_object LIKE gameobject_template; INSERT INTO saved_object SELECT * FROM gameobject_template WHERE entry=248419;
CREATE TEMPORARY TABLE gameobject_template LIKE saved_object; INSERT INTO gameobject_template SELECT * FROM saved_object;
CREATE TEMPORARY TABLE saved_spawn LIKE gameobject; INSERT INTO saved_spawn SELECT * FROM gameobject WHERE id=248419;
CREATE TEMPORARY TABLE gameobject LIKE saved_spawn; INSERT INTO gameobject SELECT * FROM saved_spawn;
CREATE TEMPORARY TABLE saved_path LIKE waypoints; INSERT INTO saved_path SELECT * FROM waypoints WHERE entry=10039704;
CREATE TEMPORARY TABLE waypoints LIKE saved_path; INSERT INTO waypoints SELECT * FROM saved_path;
CREATE TEMPORARY TABLE conditions_saved LIKE conditions;
INSERT INTO conditions_saved SELECT * FROM conditions WHERE (SourceTypeOrReferenceId=22 AND SourceEntry IN(100397,248419)) OR (SourceTypeOrReferenceId=4 AND SourceGroup=248419);
DELETE FROM conditions_saved WHERE $scope;
CREATE TEMPORARY TABLE conditions LIKE conditions_saved; INSERT INTO conditions SELECT * FROM conditions_saved;
SELECT 'before-smart',expected_smart.* FROM expected_smart ORDER BY source_type,id;
SELECT 'before-gates',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference;
SELECT 'before-object',gameobject_template.* FROM gameobject_template;
$migration
SELECT 'first-count',COUNT(*) FROM conditions WHERE $scope;
$migration
SELECT 'count',COUNT(*) FROM conditions WHERE $scope;
SELECT 'after-smart',smart_scripts.* FROM smart_scripts ORDER BY source_type,id;
SELECT 'after-gates',conditions.* FROM conditions WHERE NOT($scope) ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference;
SELECT 'after-object',gameobject_template.* FROM gameobject_template;
SELECT 'row',entryorguid,id,link,event_type,event_phase_mask,event_flags,event_param1,event_param2,action_type,action_param1,action_param2,target_type,target_x,target_y,target_z,target_o FROM smart_scripts WHERE source_type=0 AND id IN(45,50) ORDER BY id;
SELECT 'gate',SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY SourceGroup,ConditionTypeOrReference;
"@
$variants=@{
 'custom-script'="UPDATE creature_template SET ScriptName='custom_actor';"
 'custom-ai'="UPDATE creature_template SET AIName='';"
 'custom-go-script'="UPDATE gameobject_template SET ScriptName='custom_bow';"
 'custom-model'='UPDATE gameobject_template SET displayId=1;'
 'custom-loot'='UPDATE gameobject_template SET Data1=1;'
 'custom-flags'='UPDATE gameobject_template SET flags=16;'
 'custom-position'='UPDATE smart_scripts SET target_z=-50 WHERE source_type=0 AND id=50;'
 'custom-speech'='UPDATE smart_scripts SET action_param1=20 WHERE source_type=0 AND id=45;'
 'custom-timing'='UPDATE smart_scripts SET action_param2=6000 WHERE source_type=0 AND id=45;'
 'custom-waypoint'='UPDATE smart_scripts SET event_param1=15 WHERE source_type=0 AND id=45;'
 'custom-path'='UPDATE smart_scripts SET event_param2=10039705 WHERE source_type=0 AND id=45;'
 'custom-link'='UPDATE smart_scripts SET link=51 WHERE source_type=0 AND id=45;'
 'custom-credit'='UPDATE smart_scripts SET action_param2=47847 WHERE source_type=0 AND id=49;'
 'custom-phase'='UPDATE smart_scripts SET event_phase_mask=1 WHERE source_type=0 AND id=50;'
 'custom-repeat'='UPDATE smart_scripts SET event_flags=0 WHERE source_type=0 AND id=45;'
 'custom-duration'='UPDATE smart_scripts SET action_param2=300 WHERE source_type=0 AND id=50;'
 'custom-claim'='UPDATE smart_scripts SET action_param1=1 WHERE source_type=0 AND id=51;'
 'custom-take'='UPDATE smart_scripts SET action_param2=10000 WHERE source_type=0 AND id=48;'
 'custom-extra-source'='UPDATE smart_scripts SET link=50 WHERE source_type=0 AND id=44;'
 'custom-route-count'='DELETE FROM waypoints WHERE pointid=1;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`n$migration`nSELECT '$name',COUNT(*) FROM conditions WHERE $scope;`n"}
foreach($group in @(46,51)){$sql+="$reset`nINSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment) VALUES(22,$group,100397,0,1,4,1,1489,'Custom bow reveal');`n$migration`nSELECT 'custom-gate-$group',COUNT(*) FROM conditions WHERE $scope;`nSELECT 'custom-preserved-$group',COUNT(*) FROM conditions WHERE Comment='Custom bow reveal';`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Bow reveal TEMP fixture failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("first-count`t4","count`t4")+@($variants.Keys|ForEach-Object {"$_`t0"})+@(46,51|ForEach-Object {"custom-gate-$_`t1";"custom-preserved-$_`t1"})){if($rows -notcontains $expected){throw "Bow reveal mismatch: $expected"}}
foreach($kind in @('smart','gates','object')){$before=@($rows|Where-Object {$_ -like "before-$kind`t*"}|ForEach-Object {$_.Substring(8+$kind.Length)});$after=@($rows|Where-Object {$_ -like "after-$kind`t*"}|ForEach-Object {$_.Substring(7+$kind.Length)});if(Compare-Object $before $after){throw "Unexpected bow reveal $kind change"}}
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]]@($rows|Where-Object {$_ -like "row`t*"}|ForEach-Object {$_.Substring(4)}))
[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]@($rows|Where-Object {$_ -like "gate`t*"}|ForEach-Object {$_.Substring(5)}))
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/WaypointReached.inc",(Section $source '        case SMART_EVENT_WAYPOINT_REACHED:' '        case SMART_EVENT_SUMMON_DESPAWNED:'))
[IO.File]::WriteAllText("$out/SummonGo.inc",(Section $source '        case SMART_ACTION_SUMMON_GO:' '        case SMART_ACTION_KILL_UNIT:'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" /I`"$search`" /I`"$common`" `"$PSScriptRoot/MarksmanshipBowRevealTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/rows.tsv`" `"$out/conditions.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native bow reveal regression failed'}
& "$PSScriptRoot/Test-MarksmanshipReportUse.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary *> "$out/report-use-prerequisite.log"
'PASS: only three native links and four actor conditions change; model/position/duration/dialogues/credits/pickup gates remain unchanged; repeat migration and custom-data refusal pass'
