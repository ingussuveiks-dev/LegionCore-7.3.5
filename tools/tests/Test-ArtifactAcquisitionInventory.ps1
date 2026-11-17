param()
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-acquisition-inventory"
New-Item -ItemType Directory -Force $out | Out-Null
function Block([string]$source,[string]$name) {
 $a=$source.IndexOf($name); if($a -lt 0){throw "Missing production $name"}
 $b=$source.IndexOf('{',$a)+1; $depth=1
 while($depth){$depth+=([int]($source[$b] -eq '{'))-([int]($source[$b] -eq '}'));++$b}
 $source.Substring($a,$b-$a)
}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ArtifactAcquisitionInventoryCatalog.cpp -ProbeName ArtifactAcquisitionInventoryCatalog -OutputFile "$out/native.tsv"
 if($LASTEXITCODE){throw 'Native inventory catalogue failed'}
 $routes=(Get-Content "$repo/docs/audits/artifact-routes-2026-10-23.json" -Raw|ConvertFrom-Json).routes
 $items=@($routes|ForEach-Object {$_.item})
 if(@($routes).Count -ne 36 -or @($items|Sort-Object -Unique).Count -ne 36){throw 'Expected36 distinct artifact routes'}
 [IO.File]::WriteAllLines("$out/routes.txt",[string[]]$items)
 $source=Get-Content "$repo/src/server/game/Entities/Item/ArtifactAcquisitionInventory.h" -Raw
 [IO.File]::WriteAllText("$out/Inventory.inc",(Block $source 'namespace ArtifactAcquisitionInventory'))
 $source=Get-Content "$repo/src/server/game/Entities/Player/PlayerScenario.cpp" -Raw
 [IO.File]::WriteAllText("$out/EarnedReturn.inc",(Block $source 'bool Player::HasEarnedMarksmanshipReturn('))
 $gates=''
 foreach($gate in @(
  @('Rogue/Dreadblades/Dreadblades.cpp','DreadbladesAcquisition','bool Equipped(Player* player)'),
  @('Rogue/FangsoftheDevourer/Devourer.cpp','DevourerAcquisition','bool Equipped(Player* player)'),
  @('Rogue/Kingslayers/Kingslayers.cpp','KingslayersAcquisition','bool Equipped(Player* player)'),
  @('Priest/Tuure/Tuure.cpp','TuureAcquisition','bool Equipped(Player* player)'),
  @('Shaman/Sharasdal/sharasdal_acquisition.h','SharasdalAcquisition','bool Equipped(Player* player, ObjectGuid const& guid)')
 )) {
  $source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/$($gate[0])" -Raw
  $gates+="namespace $($gate[1]) {`n"+(Block $source $gate[2])+"`n}`n"
 }
 $source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/lights_wrath_pickup.h" -Raw
 $gates+='namespace LightsWrathPickup {'+(Block $source 'bool Equipped(Player* player, uint32 item)')+'}'
 [IO.File]::WriteAllText("$out/OutcomeGates.inc",$gates)
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/ArtifactAcquisitionInventoryTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`" native.tsv routes.txt`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Actual production inventory outcome regression failed'}
 & "$PSScriptRoot/Test-ArtifactForceEquip.ps1";if($LASTEXITCODE){throw 'Native249 handler regression failed'}
 # Session-local shadow: never modify live quest conditions during regression.
 $migrationPath="$repo/sql/updates/world/2026_11_17_09_artifact_acquisition_inventory.sql"
 if(!(Test-Path $migrationPath)){$migrationPath="$repo/sql/pending/2026_11_17_09_artifact_acquisition_inventory.sql"}
 $migration=Get-Content $migrationPath -Raw
 $sql=@"
CREATE TEMPORARY TABLE inventory_saved_conditions LIKE conditions;
INSERT INTO inventory_saved_conditions SELECT * FROM conditions;
-- Replay migration09 against its original six-row phase scope. Migration11
-- has a separate actual loader/admission/notification regression.
DELETE FROM inventory_saved_conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7502 AND SourceEntry=1201 AND SourceId=0 AND ElseGroup IN(3,4,5);
CREATE TEMPORARY TABLE conditions LIKE inventory_saved_conditions;
INSERT INTO conditions SELECT * FROM inventory_saved_conditions;
$migration
SELECT 'first',COUNT(*) FROM conditions WHERE ScriptName LIKE 'condition_artifact_inventory_%';
SELECT 'recovered-first',COUNT(*) FROM conditions WHERE ScriptName='condition_marksmanship_recovered_interaction';
CREATE TEMPORARY TABLE inventory_first_conditions LIKE conditions;
INSERT INTO inventory_first_conditions SELECT * FROM conditions;
$migration
SELECT 'second',COUNT(*) FROM conditions WHERE ScriptName LIKE 'condition_artifact_inventory_%';
SELECT 'recovered-second',COUNT(*) FROM conditions WHERE ScriptName='condition_marksmanship_recovered_interaction';
SELECT 'unexpected',COUNT(*) FROM conditions c LEFT JOIN inventory_saved_conditions s ON c.SourceTypeOrReferenceId=s.SourceTypeOrReferenceId AND c.SourceGroup=s.SourceGroup AND c.SourceEntry=s.SourceEntry AND c.SourceId=s.SourceId AND c.ElseGroup=s.ElseGroup AND c.ConditionTypeOrReference=s.ConditionTypeOrReference AND c.ConditionTarget=s.ConditionTarget AND c.ConditionValue1=s.ConditionValue1 AND c.ConditionValue2=s.ConditionValue2 AND c.ConditionValue3=s.ConditionValue3 AND c.NegativeCondition=s.NegativeCondition AND c.ErrorTextId=s.ErrorTextId AND c.ScriptName=s.ScriptName AND c.Comment<=>s.Comment WHERE s.SourceTypeOrReferenceId IS NULL AND c.ScriptName NOT LIKE 'condition_artifact_inventory_%' AND c.ScriptName<>'condition_marksmanship_recovered_interaction';
SELECT 'native-exit',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=60 AND SourceEntry=100397 AND ElseGroup IN(0,1);
"@
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $prior=$env:MYSQL_PWD
 try {
  $env:MYSQL_PWD=$connection[3]
  $rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE){throw 'Temporary inventory conditions SQL failed'}
 } finally {$env:MYSQL_PWD=$prior}
 [IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]$rows)
 foreach($expected in @("first`t11","second`t11","recovered-first`t2","recovered-second`t2","unexpected`t0","native-exit`t12")) {if($rows -notcontains $expected){throw "Inventory conditions mismatch: $expected"}}
 'PASS: native inventory/recovery conditions applied twice in temporary shadow; original return requirements retained'
}
