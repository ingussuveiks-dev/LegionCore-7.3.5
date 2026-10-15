$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/kings-path-movie"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_15_28_kings_path_movie.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
$migration
$migration
SELECT 'gate',ConditionTypeOrReference,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ConditionTarget FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=372 AND SourceId=13 ORDER BY ConditionTypeOrReference;
SELECT 'stale',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=372 AND SourceId=10 AND ConditionTypeOrReference=9 AND ConditionValue1=46282;
SELECT 'other',COUNT(*) FROM conditions c LEFT JOIN saved_conditions s ON c.SourceTypeOrReferenceId=s.SourceTypeOrReferenceId AND c.SourceGroup=s.SourceGroup AND c.SourceEntry=s.SourceEntry AND c.SourceId=s.SourceId AND c.ElseGroup=s.ElseGroup AND c.ConditionTypeOrReference=s.ConditionTypeOrReference AND c.ConditionValue1=s.ConditionValue1 AND c.ConditionValue2=s.ConditionValue2 AND c.ConditionValue3=s.ConditionValue3 AND c.NegativeCondition=s.NegativeCondition AND c.ConditionTarget=s.ConditionTarget WHERE c.SourceEntry<>372 AND s.SourceTypeOrReferenceId IS NULL;
SELECT 'quest',QuestID,ObjectID,Amount FROM quest_objectives WHERE QuestID=46282 AND ObjectID=119426;
SELECT 'trigger',entryorguid,source_type,action_param1 FROM smart_scripts WHERE entryorguid=372 AND source_type=13 AND id=0 AND action_type=85;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try {$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Movie condition fixture failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @("gate`t9`t46282`t0`t0`t0`t0","gate`t41`t46282`t119426`t1`t1`t0","stale`t0","other`t0","quest`t46282`t119426`t1","trigger`t372`t13`t239912")) {if($rows -notcontains $line){throw "Movie fixture mismatch: $line"}}
if(@($rows | Where-Object {$_ -like "gate`t*"}).Count -ne 2){throw 'Unexpected movie gate conditions'}
$mgr=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
if($mgr -notmatch 'SMART_SCRIPT_TYPE_EVENTOBJECT\s*=\s*13'){throw 'Native event object source changed'}
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
foreach($pair in @(@('CONDITION_QUESTTAKEN','CONDITION_QUEST_COMPLETE','QuestTaken.inc'),@('CONDITION_QUEST_OBJECTIVE_DONE','CONDITION_INSTANCE_INFO','Objective.inc'))){
 $a=$source.IndexOf('        case '+$pair[0]+':');$b=$source.IndexOf('        case '+$pair[1]+':',$a)
 if($a -lt 0 -or $b -le $a){throw 'Native quest condition branch absent'}
 [IO.File]::WriteAllText("$out/$($pair[2])",$source.Substring($a,$b-$a))
}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/KingsPathMovieTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native movie quest admission failed'}
'PASS: event source 13, real quest objective, migration replay and unrelated condition preservation'
