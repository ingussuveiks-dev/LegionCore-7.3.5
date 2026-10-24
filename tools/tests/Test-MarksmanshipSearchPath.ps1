param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-search-path"
$common="$repo/build-extractors/runtime-test/marksmanship-herald-challenge"
New-Item -ItemType Directory -Force $out | Out-Null
& "$PSScriptRoot/Test-MarksmanshipPostInquisitor.ps1" *> "$out/post-inquisitor-prerequisite.log"
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_11_marksmanship_search_path_retry.sql" -Raw
$scope='SourceTypeOrReferenceId=22 AND SourceEntry=100397 AND SourceGroup IN(43,44,62)'
$reset="DELETE FROM conditions WHERE $scope; DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart; DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actor; DELETE FROM waypoints; INSERT INTO waypoints SELECT * FROM saved_path;"
$sql=@"
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=100397 AND NOT(source_type=0 AND id=61);
UPDATE saved_smart SET action_type=53,action_param1=1,action_param2=10039704,comment='Link - Start WP' WHERE source_type=0 AND id=43;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
CREATE TEMPORARY TABLE expected_smart LIKE saved_smart;
INSERT INTO expected_smart SELECT * FROM saved_smart;
UPDATE expected_smart SET action_type=22,action_param1=1,action_param2=0,comment='Marksmanship: retain pending search path after the original Enough dialogue' WHERE source_type=0 AND id=43;
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=100397;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
INSERT INTO creature_template SELECT * FROM saved_actor;
CREATE TEMPORARY TABLE saved_path LIKE waypoints;
INSERT INTO saved_path SELECT * FROM waypoints WHERE entry=10039704;
CREATE TEMPORARY TABLE waypoints LIKE saved_path;
INSERT INTO waypoints SELECT * FROM saved_path;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100397 AND NOT($scope);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
SELECT 'before-smart',expected_smart.* FROM expected_smart ORDER BY source_type,id;
SELECT 'before-gates',conditions.* FROM conditions ORDER BY SourceGroup,ElseGroup,ConditionTypeOrReference;
SELECT 'before-path',waypoints.* FROM waypoints ORDER BY entry,pointid;
$migration
SELECT 'first-count',COUNT(*) FROM conditions WHERE $scope;
$migration
SELECT 'after-smart',smart_scripts.* FROM smart_scripts WHERE NOT(source_type=0 AND id=61) ORDER BY source_type,id;
SELECT 'after-gates',conditions.* FROM conditions WHERE NOT($scope) ORDER BY SourceGroup,ElseGroup,ConditionTypeOrReference;
SELECT 'after-path',waypoints.* FROM waypoints ORDER BY entry,pointid;
SELECT 'count',COUNT(*) FROM conditions WHERE $scope;
SELECT 'new-row',COUNT(*) FROM smart_scripts WHERE source_type=0 AND id=61;
SELECT 'row',entryorguid,id,link,event_type,event_phase_mask,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,target_type,target_param1,target_param2,target_param3 FROM smart_scripts WHERE source_type=0 AND id IN(42,43,61) ORDER BY id;
SELECT 'gate',SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY SourceGroup,ConditionTypeOrReference;
"@
$variants=@{
 'custom-script'="UPDATE creature_template SET ScriptName='custom_actor' WHERE entry=100397;"
 'custom-ai'="UPDATE creature_template SET AIName='' WHERE entry=100397;"
 'occupied-slot'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,target_type,comment) VALUES(100397,0,61,60,1,1,'Custom event');"
 'custom-phase'='UPDATE smart_scripts SET event_phase_mask=1 WHERE source_type=0 AND id=0;'
 'custom-phase-action'='UPDATE smart_scripts SET action_type=22 WHERE source_type=0 AND id=0;'
 'custom-timer'='UPDATE smart_scripts SET action_param2=8000 WHERE source_type=0 AND id=42;'
 'custom-speech'='UPDATE smart_scripts SET action_param1=18 WHERE source_type=0 AND id=42;'
 'custom-text'='UPDATE smart_scripts SET event_param1=17 WHERE source_type=0 AND id=42;'
 'custom-link'='UPDATE smart_scripts SET link=44 WHERE source_type=0 AND id=42;'
 'custom-path'='UPDATE smart_scripts SET action_param2=10039705 WHERE source_type=0 AND id=43;'
 'custom-react'='UPDATE smart_scripts SET action_param6=2 WHERE source_type=0 AND id=43;'
 'custom-repeat'='UPDATE smart_scripts SET event_flags=0 WHERE source_type=0 AND id=43;'
 'custom-poll'='UPDATE smart_scripts SET action_param2=11 WHERE source_type=0 AND id=60;'
 'custom-position'='UPDATE waypoints SET position_z=-50 WHERE pointid=1;'
 'custom-endpoint'='UPDATE waypoints SET position_z=-50 WHERE pointid=16;'
 'custom-count'='DELETE FROM waypoints WHERE pointid=5;'
 'custom-route-speech'='UPDATE smart_scripts SET action_param1=19 WHERE source_type=0 AND id=44;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`n$migration`nSELECT '$name',COUNT(*) FROM conditions WHERE $scope;`n"}
foreach($group in @(43,44,62)){$sql+="$reset`nINSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment) VALUES(22,$group,100397,0,1,4,1,1489,'Custom search admission');`n$migration`nSELECT 'custom-gate-$group',COUNT(*) FROM conditions WHERE $scope;`nSELECT 'custom-preserved-$group',COUNT(*) FROM conditions WHERE Comment='Custom search admission';`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Search-path TEMP fixture failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("first-count`t6","count`t6","new-row`t1")+@($variants.Keys|ForEach-Object {"$_`t0"})+@(43,44,62|ForEach-Object {"custom-gate-$_`t1";"custom-preserved-$_`t1"})){if($rows -notcontains $expected){throw "Search-path mismatch: $expected"}}
foreach($kind in @('smart','gates','path')){$before=@($rows|Where-Object {$_ -like "before-$kind`t*"}|ForEach-Object {$_.Substring(8+$kind.Length)});$after=@($rows|Where-Object {$_ -like "after-$kind`t*"}|ForEach-Object {$_.Substring(7+$kind.Length)});if(Compare-Object $before $after){throw "Unexpected search $kind change"}}
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]]@($rows|Where-Object {$_ -like "row`t*"}|ForEach-Object {$_.Substring(4)}))
[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]@($rows|Where-Object {$_ -like "gate`t*"}|ForEach-Object {$_.Substring(5)}))
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/AliveCondition.inc",(Section $source '        case CONDITION_ALIVE:' '        case CONDITION_HP_VAL:'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
foreach($entry in @(@('WaypointAction','        case SMART_ACTION_WP_START:','        case SMART_ACTION_WP_PAUSE:'),@('SetPhase','        case SMART_ACTION_SET_EVENT_PHASE:','        case SMART_ACTION_INC_EVENT_PHASE:'),@('FindLink','SmartScriptHolder SmartScript::FindLinkedEvent(','void SmartScript::OnUpdate('),@('SearchLinkTail','    if (e.link && e.link != e.event_id)','void SmartScript::InstallTemplate('))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",(Section $source $entry[1] $entry[2]))}
[IO.File]::WriteAllText("$out/PhaseMethods.inc",$source.Substring($source.IndexOf('bool SmartScript::IsInPhase(')))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
[IO.File]::WriteAllText("$out/StartPath.inc",(Section $source 'bool SmartAI::StartPath(' 'bool SmartAI::LoadPath('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" /I`"$common`" `"$PSScriptRoot/MarksmanshipSearchPathTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/rows.tsv`" `"$out/conditions.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native search-path regression failed'}
if($VerifyReleaseBinary){$previousPython=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_smart_waypoint_start_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical phase/waypoint PE check failed'}}finally{$env:PYTHONPATH=$previousPython}}
'PASS: guarded search phase/poll changes only native43 action and new61 plus six conditions; original speech/timing/route/reveal/credit rows preserved and custom phase/action/slot/admission refused'
