param([switch]$VerifyLiveData,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-inventory-phase"
New-Item -ItemType Directory -Force $out | Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $migrationPath="$repo/sql/updates/world/2026_11_17_11_ebonchill_inventory_phase.sql"
 if(!(Test-Path $migrationPath)){$migrationPath="$repo/sql/pending/2026_11_17_11_ebonchill_inventory_phase.sql"}
 $migration=Get-Content $migrationPath -Raw
 $scope='SourceTypeOrReferenceId=23 AND SourceGroup=7502 AND SourceEntry=1201 AND SourceId=0'
 $reset="DELETE FROM conditions;INSERT INTO conditions SELECT * FROM saved_phase_conditions WHERE ElseGroup<3;UPDATE conditions SET ConditionTypeOrReference=32,ConditionValue1=16,ScriptName='condition_artifact_inventory_128862' WHERE ConditionTypeOrReference=3;"
 $sql=@"
CREATE TEMPORARY TABLE saved_phase_conditions LIKE conditions;
INSERT INTO saved_phase_conditions SELECT * FROM conditions WHERE $scope;
CREATE TEMPORARY TABLE conditions LIKE saved_phase_conditions;
$reset
$migration
SELECT 'A',ElseGroup,ConditionTypeOrReference,ConditionValue1,NegativeCondition,ScriptName FROM conditions ORDER BY ElseGroup,ConditionTypeOrReference;
SELECT 'count-first',COUNT(*) FROM conditions;
$migration
SELECT 'count-second',COUNT(*) FROM conditions;
$reset
UPDATE conditions SET ScriptName='custom' WHERE ElseGroup=0 AND ConditionTypeOrReference=32;
$migration
SELECT 'foreign-script',COUNT(*) FROM conditions;
$reset
DELETE FROM conditions WHERE ElseGroup=1 AND ConditionTypeOrReference=8;
$migration
SELECT 'missing-quest',COUNT(*) FROM conditions;
$reset
INSERT INTO conditions SELECT SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,3,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment FROM conditions WHERE ElseGroup=0;
$migration
SELECT 'foreign-branch',COUNT(*) FROM conditions;
SELECT 'EOF';
"@
 $config=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $prior=$env:MYSQL_PWD
 try {
  $env:MYSQL_PWD=$config[3]
  [IO.File]::WriteAllText("$out/fixture.sql",$sql)
  $exit=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($config[0])","--port=$($config[1])","--user=$($config[2])","--database=$($config[4])",'--batch','--raw','--skip-column-names') -InputFile "$out/fixture.sql" -OutputFile "$out/fixture.stdout" -ErrorFile "$out/fixture.stderr"
  if($exit -or (Get-Item "$out/fixture.stderr").Length){throw 'Phase temporary SQL fixture failed'}
  $rows=@(Get-Content "$out/fixture.stdout")
  foreach($expected in @("count-first`t15","count-second`t15","foreign-script`t6","missing-quest`t5","foreign-branch`t8",'EOF')){if($rows -notcontains $expected){throw "Phase outcome differs: $expected"}}
  [IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]@($rows|Where-Object {$_ -like "A`t*"}|ForEach-Object {$_.Substring(2)}))
  if($VerifyLiveData){
   $live=@("SELECT ElseGroup,ConditionTypeOrReference,ConditionValue1,NegativeCondition,ScriptName FROM conditions WHERE $scope ORDER BY ElseGroup,ConditionTypeOrReference;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($config[0]) --port=$($config[1]) --user=$($config[2]) --database=$($config[4]) --batch --raw --skip-column-names)
   if($LASTEXITCODE -or (Compare-Object $live (Get-Content "$out/conditions.tsv"))){throw 'Live phase data differs from tested migration'}
  }
 } finally {$env:MYSQL_PWD=$prior}
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/artifact_inventory_phase.py" "$out"
 if($LASTEXITCODE){throw 'Production phase source preparation failed'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/ArtifactInventoryPhaseTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/conditions.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native inventory phase regression failed'}
 if($VerifyReleaseBinary){
  $oldPythonPath=$env:PYTHONPATH
  try {
   $env:PYTHONPATH="$repo/.codex/pe-review-deps;$oldPythonPath"
   & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_artifact_inventory_phase_release.py" "$repo/build-extractors/bin/Release"
   if($LASTEXITCODE){throw 'Linked inventory phase notifications differ'}
  } finally {$env:PYTHONPATH=$oldPythonPath}
 }
 'PASS exact phase migration twice and foreign/missing branch refusal; actual whitelist, condition dispatch, inventory policy and change notifications; client visibility pending'
}
