param([string]$MigrationFile="$PSScriptRoot/../../sql/updates/world/2026_10_15_33_dark_whispers_arrival.sql",[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/dark-whispers-arrival"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Extract([string]$s,[string]$start,[string]$end){$a=$s.IndexOf($start);$b=$s.IndexOf($end,$a);if($a -lt 0 -or $b -lt 0){throw "Missing production section: $start"};return $s.Substring($a,$b-$a)}
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/dark_whispers_artifact.cpp" -Raw
[IO.File]::WriteAllText("$out/Arrival.inc",(Extract $source 'namespace DarkWhispersArrival' 'class instance_dark_whispers_artifact'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/LosEvent.inc",(Extract $source.Substring($source.IndexOf('void SmartScript::ProcessEvent(')) '        case SMART_EVENT_OOC_LOS:' '        case SMART_EVENT_IC_LOS:'))
$source=Get-Content "$repo/src/server/game/AI/CreatureAI.cpp" -Raw
[IO.File]::WriteAllText("$out/SafeLos.inc",(Extract $source 'void CreatureAI::MoveInLineOfSight_Safe(' 'bool CreatureAI::CanSeeEvenInPassiveMode()'))
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/PlayerCondition.inc",(Extract $source '        case CONDITION_OBJECT_ENTRY:' '        case CONDITION_TYPE_MASK:'))
$migration=Get-Content $MigrationFile -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=99739;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
$migration
SELECT 'FIRST';
SELECT SourceGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2 FROM conditions ORDER BY SourceGroup,ConditionTypeOrReference;
$migration
SELECT 'SECOND';
SELECT SourceGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2 FROM conditions ORDER BY SourceGroup,ConditionTypeOrReference;
SELECT 'EVENT',id,link,event_type,event_flags,event_param1,event_param2,action_type,action_param1,action_param2,target_type,target_param1 FROM smart_scripts WHERE entryorguid=99739 AND source_type=0 AND id=1;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Arrival SQL fixture failed'}}finally{$env:MYSQL_PWD=$old}
$a=[Array]::IndexOf($rows,'FIRST');$b=[Array]::IndexOf($rows,'SECOND');$d=$rows.Count-1
$first=@($rows[($a+1)..($b-1)]);$second=@($rows[($b+1)..($d-1)])
if(($first -join "`n") -ne ($second -join "`n")){throw 'Arrival migration is not idempotent'}
foreach($expected in @("1`t44`t1`t1097`t0","2`t44`t1`t1097`t2","2`t31`t0`t4`t0")){if($first -notcontains $expected){throw "Missing arrival condition: $expected"}}
if($rows[$d] -ne "EVENT`t1`t2`t10`t1`t1`t30`t205`t92`t50918`t21`t30"){throw 'Native one-shot LOS/credit/wave chain changed; review fixture'}
[IO.File]::WriteAllText("$out/ConditionValues.inc",'constexpr uint32 CONDITION_OBJECT_ENTRY=31, ConditionValue1=4, ConditionValue2=0;')
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/DarkWhispersArrivalTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Dark Whispers arrival regression failed'}
'PASS: SQL replay preserves native actor/step gates and requires a Player invoker before consumption'
if($VerifyReleaseBinary){
    $binary=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe"))
    if(!$binary.Contains('Dark Whispers: barrier arrival credited player')){throw 'Canonical Release lacks arrival retry'}
    'PASS: canonical Release contains barrier arrival recovery'
}
