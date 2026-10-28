param([string]$MigrationPath,[switch]$AddressSanitizer,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/stromkar-ritual-stages"
if(!$MigrationPath){$MigrationPath="$repo/sql/updates/world/2026_10_28_01_stromkar_ritual_beam_stages.sql"}
function Section([string]$s,[string]$start,[string]$end){$a=$s.IndexOf($start);$b=$s.IndexOf($end,[Math]::Max(0,$a));if($a -lt 0 -or $b -le $a){throw "Missing $start"};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $migration=Get-Content $MigrationPath -Raw
 $scope='SourceTypeOrReferenceId=22 AND SourceGroup=7 AND SourceEntry=101875 AND SourceId=0'
 $reset="DELETE FROM conditions;INSERT INTO conditions SELECT * FROM saved_conditions;DELETE FROM smart_scripts;INSERT INTO smart_scripts SELECT * FROM saved_smart;DELETE FROM creature_template;INSERT INTO creature_template SELECT * FROM saved_actor;"
 $sql=@"
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry IN(101875,103144);
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid IN(101875,103144);
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry IN(101875,103144);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
"@
 if($VerifyLiveData){$sha=(Get-FileHash -LiteralPath $MigrationPath -Algorithm SHA1).Hash;$sql+="`nUPDATE saved_conditions SET ElseGroup=0 WHERE $scope AND ElseGroup=1 AND ConditionTypeOrReference=44 AND ConditionTarget=1 AND ConditionValue1=1037 AND ConditionValue2=0 AND EXISTS(SELECT 1 FROM updates WHERE name='2026_10_28_01_stromkar_ritual_beam_stages.sql' AND hash='$sha' AND state='RELEASED');`n"}
 $sql+="$reset`nSELECT 'smart-before',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;SELECT 'other-before',conditions.* FROM conditions WHERE NOT($scope) ORDER BY SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference;`n$migration`nSELECT 'first',conditions.* FROM conditions ORDER BY SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference;`n$migration`nSELECT 'second',conditions.* FROM conditions ORDER BY SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference;SELECT 'smart-after',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;SELECT 'other-after',conditions.* FROM conditions WHERE NOT($scope) ORDER BY SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference;SELECT 'row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY ConditionValue2;`n"
 $variants=[ordered]@{
  'custom-ai'="UPDATE creature_template SET ScriptName='custom' WHERE entry=101875;"
  'custom-spell'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=101875 AND id=6;'
  'custom-target'='UPDATE smart_scripts SET target_param1=1 WHERE entryorguid=101875 AND id=6;'
  'custom-flags'='UPDATE smart_scripts SET event_flags=0 WHERE entryorguid=101875 AND id=6;'
  'custom-difficulty'="UPDATE smart_scripts SET Difficulties='12' WHERE entryorguid=101875 AND id=6;"
  'custom-gate-script'="UPDATE conditions SET ScriptName='custom' WHERE $scope;"
  'custom-gate-target'="UPDATE conditions SET ConditionTarget=0 WHERE $scope;"
  'custom-gate-group'="UPDATE conditions SET ElseGroup=2 WHERE $scope;"
  'missing-stage'="DELETE FROM conditions WHERE $scope AND ConditionValue2=1;"
  'extra-gate'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionTarget,ConditionValue1) VALUES(22,7,101875,22,1,1539);"
 }
 foreach($v in $variants.Keys){$sql+="$reset`n$($variants[$v])`nSELECT '$v-before',conditions.* FROM conditions ORDER BY SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference;`n$migration`nSELECT '$v-after',conditions.* FROM conditions ORDER BY SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference;`n"}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 function Query([string]$query){$prior=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$rows=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Ritual stage TEMP query failed'};$rows}finally{$env:MYSQL_PWD=$prior}}
 $rows=Query $sql;[IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 function Snapshot([string]$name){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$name`t*"}|ForEach-Object {$_.Substring($name.Length+1)}))}
 foreach($pair in @(@('first','second'),@('smart-before','smart-after'),@('other-before','other-after'))){if((Snapshot $pair[0]) -cne (Snapshot $pair[1])){throw "Changed $pair"}}
 foreach($v in $variants.Keys){if((Snapshot "$v-before") -cne (Snapshot "$v-after")){throw "Changed custom $v"}}
 $data=@($rows|Where-Object {$_ -like "row`t*"}|ForEach-Object {$_.Substring(4)});if($data.Count -ne 2 -or $data -notcontains "1`t44`t1`t1037`t0`t0`t0" -or $data -notcontains "0`t44`t1`t1037`t1`t0`t0"){throw 'Expected two exact alternative stage rules'}
 [IO.File]::WriteAllLines("$out/rows.tsv",[string[]]$data)
 $source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
 [IO.File]::WriteAllText("$out/ScenarioCondition.inc",(Section $source '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:'))
 [IO.File]::WriteAllText("$out/ElseGroups.inc",(Section $source 'bool ConditionMgr::IsObjectMeetToConditionList(' 'bool ConditionMgr::IsObjectMeetToConditions(WorldObject* object,'))
 [IO.File]::WriteAllText("$out/SmartConditions.inc",(Section $source 'bool ConditionMgr::IsObjectMeetingSmartEventConditions(' 'bool Condition::isLoaded()'))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $flags=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /W4 $flags /I`"$out`" `"$PSScriptRoot/StromkarRitualStagesTest.cpp`" /Fe:test.exe /Fo:test.obj /Fd:test.pdb`nif errorlevel 1 exit /b 1`ntest.exe rows.tsv`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native ritual stage regression failed'}
 if($VerifyLiveData){$live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_28_01_stromkar_ritual_beam_stages.sql'; SELECT 'row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY ConditionValue2;";if($live -notcontains "release`t$sha`tRELEASED"){throw 'Live ritual migration hash differs'};$actual=@($live|Where-Object {$_ -like "row`t*"}|ForEach-Object {$_.Substring(4)});if(Compare-Object $data $actual){throw 'Live ritual alternatives differ'}}
 'PASS guarded ritual stage SQL twice/custom refusals and native source22 predicate; native casting/rendering remain separate checks'
}
