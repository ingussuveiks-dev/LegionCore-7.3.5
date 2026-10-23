param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-orestes-farewell"
New-Item -ItemType Directory -Force $out | Out-Null
& "$PSScriptRoot/Test-MarksmanshipOrestes.ps1" *> "$out/discovery.log"
Get-ChildItem "$repo/build-extractors/runtime-test/marksmanship-orestes" -Filter '*.inc' | Copy-Item -Destination $out
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_04_marksmanship_orestes_credit_retry.sql" -Raw
$reset='DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM baseline_smart; DELETE FROM conditions; INSERT INTO conditions SELECT * FROM baseline_conditions;'
$sql=@"
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid IN(100398,100749);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
DELETE FROM smart_scripts WHERE entryorguid=100398 AND ((source_type=0 AND id IN(5,6)) OR (source_type=9 AND id=1));
UPDATE smart_scripts SET action_type=205,action_param1=92,action_param2=48071,target_type=21,target_param1=50,comment='TS - CSC' WHERE entryorguid=100398 AND source_type=9 AND id=0;
INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,event_flags,action_type,target_type,comment) VALUES(100398,9,1,0,1,37,1,'TS - Die');
CREATE TEMPORARY TABLE baseline_smart LIKE smart_scripts;
INSERT INTO baseline_smart SELECT * FROM smart_scripts;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100398;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions WHERE NOT (SourceId=0 AND SourceGroup IN(6,7));
CREATE TEMPORARY TABLE baseline_conditions LIKE conditions;
INSERT INTO baseline_conditions SELECT * FROM conditions;
SELECT 'before',smart_scripts.* FROM smart_scripts WHERE NOT (entryorguid=100398 AND source_type=9) ORDER BY entryorguid,source_type,id;
$migration
$migration
SELECT 'after',smart_scripts.* FROM smart_scripts WHERE NOT (entryorguid=100398 AND (source_type=9 OR (source_type=0 AND id IN(5,6)))) ORDER BY entryorguid,source_type,id;
SELECT 'row',source_type,id,link,event_type,event_phase_mask,event_flags,event_param1,event_param2,action_type,action_param1,action_param2,action_param3,action_param4,target_type,target_param1 FROM smart_scripts WHERE entryorguid=100398 AND ((source_type=0 AND id IN(5,6)) OR source_type=9) ORDER BY source_type,id;
SELECT 'gate',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100398 AND SourceGroup=6 AND SourceId=0 ORDER BY ElseGroup,ConditionTypeOrReference;
SELECT 'count',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100398 AND SourceGroup=6 AND SourceId=0;
SELECT 'timed-count',COUNT(*) FROM smart_scripts WHERE entryorguid=100398 AND source_type=9;
$reset
"@
$variants=@{
 'occupied-slot'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,target_type,comment) VALUES(100398,0,5,61,37,1,'Custom');"
 'occupied-link'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,target_type,comment) VALUES(100398,0,6,61,37,1,'Custom');"
 'custom-phase'='UPDATE smart_scripts SET event_phase_mask=2 WHERE entryorguid=100398 AND source_type=0 AND id=0;'
 'custom-phase-action'='UPDATE smart_scripts SET action_type=30 WHERE entryorguid=100398 AND source_type=0 AND id=0;'
 'custom-timer'='UPDATE smart_scripts SET event_param1=12000,event_param2=12000 WHERE entryorguid=100398 AND source_type=9 AND id=0;'
 'custom-credit'='UPDATE smart_scripts SET action_param2=49592 WHERE entryorguid=100398 AND source_type=9 AND id=0;'
 'custom-target'='UPDATE smart_scripts SET target_param1=60 WHERE entryorguid=100398 AND source_type=9 AND id=0;'
 'custom-death'='UPDATE smart_scripts SET action_type=41 WHERE entryorguid=100398 AND source_type=9 AND id=1;'
 'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ConditionTypeOrReference,ConditionValue1,Comment) VALUES(22,6,100398,0,9,40419,'Custom');"
 'extra-timed-row'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,target_type,comment) VALUES(100398,9,2,0,37,1,'Custom');"
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`n$migration`nSELECT '$name',COUNT(*) FROM smart_scripts WHERE entryorguid=100398 AND source_type=9 AND id=1;`n"}
$sql+="$reset`nCREATE TEMPORARY TABLE saved_actor LIKE creature_template; INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=100398; CREATE TEMPORARY TABLE creature_template LIKE saved_actor; INSERT INTO creature_template SELECT * FROM saved_actor; UPDATE creature_template SET ScriptName='custom_actor';`n$migration`nSELECT 'custom-script',COUNT(*) FROM smart_scripts WHERE entryorguid=100398 AND source_type=9 AND id=1;"
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Orestes farewell temporary SQL fixture failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("count`t8","timed-count`t1","custom-script`t1")+@($variants.Keys | ForEach-Object {"$_`t1"})){if($rows -notcontains $expected){throw "Orestes farewell mismatch: $expected"}}
$before=@($rows | Where-Object {$_ -like "before`t*"} | ForEach-Object {$_.Substring(7)})
$after=@($rows | Where-Object {$_ -like "after`t*"} | ForEach-Object {$_.Substring(6)})
if(Compare-Object $before $after){throw 'Unrelated Orestes/Torvis fields changed'}
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]]@($rows | Where-Object {$_ -like "row`t*"} | ForEach-Object {$_.Substring(4)}))
[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]@($rows | Where-Object {$_ -like "gate`t*"} | ForEach-Object {$_.Substring(5)}))
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/QuestConditions.inc",(Section $source '        case CONDITION_QUESTTAKEN:' '        case CONDITION_QUEST_NONE:')+(Section $source '        case CONDITION_ALIVE:' '        case CONDITION_HP_VAL:'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$sections=@{
 'SetPhase'=@('        case SMART_ACTION_SET_EVENT_PHASE:','        case SMART_ACTION_INC_EVENT_PHASE:')
 'Die'=@('        case SMART_ACTION_DIE:','        case SMART_ACTION_SET_IN_COMBAT_WITH_ZONE:')
 'TimedUpdate'=@('        case SMART_EVENT_UPDATE:','        case SMART_EVENT_UPDATE_OOC:')
 'UpdateTimer'=@('void SmartScript::UpdateTimer(','bool SmartScript::CheckTimer(')
}
foreach($name in $sections.Keys){[IO.File]::WriteAllText("$out/$name.inc",(Section $source $sections[$name][0] $sections[$name][1]))}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/MarksmanshipOrestesFarewellTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/rows.tsv`" `"$out/conditions.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Orestes farewell regression failed'}
if($VerifyReleaseBinary){
 $previousPython=$env:PYTHONPATH
 try{
  $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython"
  & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_orestes_farewell_release.py" "$repo/build-extractors/bin/Release"
  if($LASTEXITCODE){throw 'Canonical Orestes farewell PE verification failed'}
 }finally{$env:PYTHONPATH=$previousPython}
}
'PASS: twice-applied native-only migration retains discovery/Torvis rows and refuses occupied slots, custom phases/timer/credit/target/death/conditions/script; no live character changes'
