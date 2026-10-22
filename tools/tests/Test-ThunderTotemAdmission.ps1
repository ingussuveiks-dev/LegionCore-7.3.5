$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/thunder-totem-admission"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_22_01_thunder_totem_admission.sql" -Raw
$sql=@'
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
'@
$select=@'
SELECT 'gate',SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions
 WHERE (SourceTypeOrReferenceId=22 AND SourceId=13 AND SourceGroup=1 AND SourceEntry IN(309,368))
 OR (SourceTypeOrReferenceId=18 AND SourceGroup IN(107988,108081,117453,117520))
 OR (SourceTypeOrReferenceId=22 AND SourceId=0 AND SourceGroup=1 AND SourceEntry IN(117453,117520)) ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference;
SELECT 'stale',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceId=10 AND SourceGroup=1 AND SourceEntry IN(309,368);
'@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$c[3]
 $rows=@(($sql+"`n"+$migration+"`n"+$select+"`n"+$migration+"`n"+$select) | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Thunder Totem temporary migration replay failed'}
}finally{$env:MYSQL_PWD=$old}
if(@($rows|Where-Object {$_ -eq "stale`t0"}).Count -ne 2){throw 'Wrong-source conversation conditions remain'}
$gates=@($rows|Where-Object {$_ -like "gate`t*"}|Select-Object -Unique)
if($gates.Count -ne 13){throw "Unexpected admission gate count: $($gates.Count)"}
foreach($gate in $gates){if(@($rows|Where-Object {$_ -eq $gate}).Count -ne 2){throw 'Migration is not repeatable'}}
[IO.File]::WriteAllLines("$out/gates.tsv",$gates)
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
foreach($pair in @(@('Taken','        case CONDITION_QUESTTAKEN:','        case CONDITION_QUEST_COMPLETE:'),@('Objective','        case CONDITION_QUEST_OBJECTIVE_DONE:','        case CONDITION_INSTANCE_INFO:'),@('Lookup','ConditionList ConditionMgr::GetConditionsForSmartEvent(','ConditionList ConditionMgr::GetConditionsForNpcVendorEvent('))){
 $a=$source.IndexOf($pair[1]);$b=$source.IndexOf($pair[2],$a)
 [IO.File]::WriteAllText("$out/$($pair[0]).inc",$source.Substring($a,$b-$a))
}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/ThunderTotemAdmissionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif %errorlevel% neq 0 exit /b %errorlevel%`n`"$out/test.exe`" `"$out/gates.tsv`"`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native Thunder Totem condition admission failed'}
