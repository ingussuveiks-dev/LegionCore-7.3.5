param([string]$MigrationFile="$PSScriptRoot/../../sql/updates/world/2026_10_15_26_smart_scenario_actor_conditions.sql")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/smart-scenario-actor"
New-Item -ItemType Directory -Force -Path $out | Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
$migration=Get-Content $MigrationFile -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
-- Reconstruct historical actor-only input; later intentional player/base
-- pairs must not be collapsed into duplicate keys. Every current row is
-- independently checked through the native target binding below.
INSERT INTO conditions SELECT * FROM saved_conditions
WHERE NOT(SourceTypeOrReferenceId=22 AND SourceId IN(0,1,9)
AND ConditionTypeOrReference=44 AND ConditionTarget<>1);
UPDATE conditions SET ConditionTarget=0 WHERE SourceTypeOrReferenceId=22 AND SourceId IN(0,1,9) AND ConditionTypeOrReference=44;
$migration
$migration
SELECT 'row',SourceEntry,SourceId,SourceGroup,ConditionValue1,ConditionValue2,NegativeCondition,ConditionTarget FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceId IN(0,1,9) AND ConditionTypeOrReference=44;
SELECT 'current',SourceEntry,SourceId,SourceGroup,ConditionValue1,ConditionValue2,NegativeCondition,ConditionTarget FROM saved_conditions WHERE SourceTypeOrReferenceId=22 AND SourceId IN(0,1,9) AND ConditionTypeOrReference=44;
SELECT 'unchanged_other',COUNT(*) FROM conditions c LEFT JOIN saved_conditions s ON c.SourceTypeOrReferenceId=s.SourceTypeOrReferenceId AND c.SourceGroup=s.SourceGroup AND c.SourceEntry=s.SourceEntry AND c.SourceId=s.SourceId AND c.ElseGroup=s.ElseGroup AND c.ConditionTypeOrReference=s.ConditionTypeOrReference AND c.ConditionValue1=s.ConditionValue1 AND c.ConditionValue2=s.ConditionValue2 AND c.ConditionValue3=s.ConditionValue3 AND c.NegativeCondition=s.NegativeCondition AND c.ConditionTarget=s.ConditionTarget WHERE NOT(c.SourceTypeOrReferenceId=22 AND c.SourceId IN(0,1,9) AND c.ConditionTypeOrReference=44) AND s.SourceTypeOrReferenceId IS NULL;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try {$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Condition migration fixture failed'}}finally{$env:MYSQL_PWD=$old}
if($rows -notcontains "unchanged_other`t0"){throw 'Scene or unrelated condition changed'}
[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]@($rows | Where-Object {$_ -like "row`t*"} | ForEach-Object {$_.Substring(4)}))
[IO.File]::WriteAllLines("$out/current.tsv",[string[]]@($rows | Where-Object {$_ -like "current`t*"} | ForEach-Object {$_.Substring(8)}))
$s=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$a=$s.IndexOf('        case CONDITION_SCENARION_STEP:');$b=$s.IndexOf('        case CONDITION_REALM_ACHIEVEMENT:',$a)
[IO.File]::WriteAllText("$out/ScenarioCondition.inc",$s.Substring($a,$b-$a))
$binding=[regex]::Match($s,'ConditionSourceInfo sourceInfo\(unit, baseObject\);')
if(!$binding.Success){throw 'Native initial SmartEvent gate binding changed'}
[IO.File]::WriteAllText("$out/SmartTargetBinding.inc",$binding.Value)
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$a=$s.IndexOf('void SmartScript::ProcessEventsFor(');$b=$s.IndexOf('void SmartScript::ProcessAction(',$a)
$dispatch=$s.Substring($a,$b-$a)
if($dispatch.IndexOf('IsObjectMeetingSmartEventConditions') -gt $dispatch.IndexOf('ProcessEvent(mEvent')){throw 'Initial condition gate no longer precedes ProcessEvent'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/SmartScenarioActorTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/conditions.tsv`" `"$out/current.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native scenario actor condition regression failed'}
}
