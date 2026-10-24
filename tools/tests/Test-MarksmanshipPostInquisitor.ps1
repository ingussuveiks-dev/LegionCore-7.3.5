param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-post-inquisitor"
$common="$repo/build-extractors/runtime-test/marksmanship-herald-challenge"
New-Item -ItemType Directory -Force $out | Out-Null
& "$PSScriptRoot/Test-MarksmanshipInquisitorAdmission.ps1" *> "$out/inquisitor-prerequisite.log"
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_10_marksmanship_post_inquisitor_retry.sql" -Raw
$scope='SourceTypeOrReferenceId=22 AND SourceEntry=100397 AND SourceGroup IN(40,61)'
$reset="DELETE FROM conditions WHERE $scope; DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart; DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actor; DELETE FROM creature; INSERT INTO creature SELECT * FROM saved_spawn; DELETE FROM waypoints; INSERT INTO waypoints SELECT * FROM saved_path;"
$sql=@"
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid IN(100397,101269) AND NOT(entryorguid=100397 AND source_type=0 AND id=60);
UPDATE saved_smart SET action_type=53,action_param2=10039704,comment='Link - Start WP'
WHERE entryorguid=100397 AND source_type=0 AND id=43 AND action_type=22 AND action_param1=1 AND action_param2=0
AND comment='Marksmanship: retain pending search path after the original Enough dialogue';
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry IN(100397,101269);
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
INSERT INTO creature_template SELECT * FROM saved_actor;
CREATE TEMPORARY TABLE saved_spawn LIKE creature;
INSERT INTO saved_spawn SELECT * FROM creature WHERE guid=370758;
CREATE TEMPORARY TABLE creature LIKE saved_spawn;
INSERT INTO creature SELECT * FROM saved_spawn;
CREATE TEMPORARY TABLE saved_path LIKE waypoints;
INSERT INTO saved_path SELECT * FROM waypoints WHERE entry=10039705;
CREATE TEMPORARY TABLE waypoints LIKE saved_path;
INSERT INTO waypoints SELECT * FROM saved_path;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry IN(100397,101269) AND NOT($scope);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
SELECT 'before-smart',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT 'before-gates',conditions.* FROM conditions ORDER BY SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference;
SELECT 'before-path',waypoints.* FROM waypoints ORDER BY entry,pointid;
$migration
SELECT 'first-count',COUNT(*) FROM conditions WHERE $scope;
$migration
SELECT 'after-smart',smart_scripts.* FROM smart_scripts WHERE NOT(entryorguid=100397 AND source_type=0 AND id=60) ORDER BY entryorguid,source_type,id;
SELECT 'after-gates',conditions.* FROM conditions WHERE NOT($scope) ORDER BY SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference;
SELECT 'after-path',waypoints.* FROM waypoints ORDER BY entry,pointid;
SELECT 'count',COUNT(*) FROM conditions WHERE $scope;
SELECT 'new-row',COUNT(*) FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=60;
SELECT 'row',entryorguid,id,link,event_type,event_phase_mask,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,target_type,target_param1,target_param2,target_param3 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id IN(39,60) ORDER BY id;
SELECT 'gate',SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY SourceGroup,ConditionTypeOrReference;
"@
$variants=@{
 'custom-script'="UPDATE creature_template SET ScriptName='custom_actor' WHERE entry=100397;"
 'custom-ai'="UPDATE creature_template SET AIName='' WHERE entry=101269;"
 'occupied-slot'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,target_type,comment) VALUES(100397,0,60,60,1,1,'Custom event');"
 'custom-phase'='UPDATE smart_scripts SET event_phase_mask=1 WHERE entryorguid=100397 AND id=39;'
 'custom-repeat'='UPDATE smart_scripts SET event_flags=0 WHERE entryorguid=100397 AND id=39;'
 'custom-signal'='UPDATE smart_scripts SET event_param2=11 WHERE entryorguid=100397 AND id=39;'
 'custom-link'='UPDATE smart_scripts SET link=41 WHERE entryorguid=100397 AND id=39;'
 'custom-path'='UPDATE smart_scripts SET action_param2=10039704 WHERE entryorguid=100397 AND id=39;'
 'custom-react'='UPDATE smart_scripts SET action_param6=2 WHERE entryorguid=100397 AND id=39;'
 'custom-speech'='UPDATE smart_scripts SET action_param2=9000 WHERE entryorguid=100397 AND id=40;'
 'custom-cleanup'='UPDATE smart_scripts SET action_param1=46957 WHERE entryorguid=100397 AND id=41;'
 'custom-credit'='UPDATE smart_scripts SET action_param2=49655 WHERE entryorguid=101269 AND id=10;'
 'custom-target'='UPDATE smart_scripts SET target_type=7 WHERE entryorguid=101269 AND id=12;'
 'custom-position'='UPDATE waypoints SET position_z=-50 WHERE entry=10039705;'
 'custom-map'='UPDATE creature SET map=0 WHERE guid=370758;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`n$migration`nSELECT '$name',COUNT(*) FROM conditions WHERE $scope;`n"}
foreach($group in @(40,61)){$sql+="$reset`nINSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment) VALUES(22,$group,100397,0,1,4,1,1489,'Custom post-Inquisitor admission');`n$migration`nSELECT 'custom-gate-$group',COUNT(*) FROM conditions WHERE $scope;`nSELECT 'custom-preserved-$group',COUNT(*) FROM conditions WHERE Comment='Custom post-Inquisitor admission';`nSELECT 'custom-no-row-$group',COUNT(*) FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=60;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Post-Inquisitor temporary fixture failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("first-count`t4","count`t4","new-row`t1","custom-gate-40`t1","custom-gate-61`t1","custom-preserved-40`t1","custom-preserved-61`t1","custom-no-row-40`t0","custom-no-row-61`t0")+@($variants.Keys|ForEach-Object {"$_`t0"})){if($rows -notcontains $expected){throw "Post-Inquisitor mismatch: $expected"}}
foreach($kind in @('smart','gates','path')){
 $before=@($rows|Where-Object {$_ -like "before-$kind`t*"}|ForEach-Object {$_.Substring(8+$kind.Length)})
 $after=@($rows|Where-Object {$_ -like "after-$kind`t*"}|ForEach-Object {$_.Substring(7+$kind.Length)})
 if(Compare-Object $before $after){throw "Original post-Inquisitor $kind fields changed"}
}
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]]@($rows|Where-Object {$_ -like "row`t*"}|ForEach-Object {$_.Substring(4)}))
[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]@($rows|Where-Object {$_ -like "gate`t*"}|ForEach-Object {$_.Substring(5)}))
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/AliveCondition.inc",(Section $source '        case CONDITION_ALIVE:' '        case CONDITION_HP_VAL:'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/WaypointAction.inc",(Section $source '        case SMART_ACTION_WP_START:' '        case SMART_ACTION_WP_PAUSE:'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
[IO.File]::WriteAllText("$out/StartPath.inc",(Section $source 'bool SmartAI::StartPath(' 'bool SmartAI::LoadPath('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" /I`"$common`" `"$PSScriptRoot/MarksmanshipPostInquisitorTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/rows.tsv`" `"$out/conditions.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native post-Inquisitor regression failed'}
if($VerifyReleaseBinary){
 $previousPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_smart_waypoint_start_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical waypoint PE verification failed'}}finally{$env:PYTHONPATH=$previousPython}
}
'PASS: isolated stage6 sender and alive/stage gates preserve every original actor/path/credit/dialogue/cleanup field, apply twice and refuse custom definitions'
