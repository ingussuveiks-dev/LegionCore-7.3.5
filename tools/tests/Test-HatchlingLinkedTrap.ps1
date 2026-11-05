param([switch]$Baseline,[switch]$VerifyLiveData,[string]$MigrationPath)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$MigrationPath){$MigrationPath="$repo/sql/updates/world/2026_11_05_04_hatchling_linked_trap.sql";if(!(Test-Path -LiteralPath $MigrationPath)){$MigrationPath="$repo/.codex/pending-sql/2026_11_05_04_hatchling_linked_trap.sql"}}
$out="$repo/build-extractors/runtime-test/hatchling-linked-trap/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force "$out/logs"|Out-Null
$migration=Get-Content -LiteralPath $MigrationPath -Raw;$commands=@((($migration -split "`n"|Where-Object {$_ -notmatch '^--'}) -join "`n").Split(';')|Where-Object {$_.Trim()}|ForEach-Object {'R"HATCHLING_SQL('+$_+')HATCHLING_SQL"'});[IO.File]::WriteAllText("$out/HatchlingMigration.inc",($commands -join ",`n"))
$configPath="$repo/build-extractors/bin/Release/worldserver.conf";$configHash=(Get-FileHash $configPath).Hash;$config=Get-Content $configPath -Raw;$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'));[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$inputs=@('tools/tests/HatchlingLinkedTrapNative.cpp','tools/tests/Test-HatchlingLinkedTrap.ps1','src/server/game/Globals/ObjectMgr.cpp','src/server/game/Entities/GameObject/GameObjectData.h','src/server/game/Entities/GameObject/GameObject.cpp','src/server/game/Spells/SpellEffects.cpp','build-extractors/src/server/game/Release/game.lib','build-extractors/bin/Release/worldserver.exe');$hashes=@{};foreach($p in $inputs){$hashes[$p]=(Get-FileHash "$repo/$p").Hash}
$oldConfig=$env:LEGION_DIALOGUE_CONFIG;$oldCompiler=$env:CL
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$oldCompiler /DNDEBUG /I`"$out`""
 if($Baseline){$env:CL+=' /DHATCHLING_LINKED_BASELINE'}elseif($VerifyLiveData){$env:CL+=' /DHATCHLING_LINKED_LIVE'}
 try{& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  if(!$Baseline){
   $live=if($VerifyLiveData){"SELECT 'live',Data7,Data1,Data14 FROM gameobject_template WHERE entry=214862 AND type=3;"}else{''}
   $sql=@"
$live
CREATE TEMPORARY TABLE hatchling_guard_seed LIKE gameobject_template;
CREATE TEMPORARY TABLE gameobject_template LIKE hatchling_guard_seed;
INSERT INTO gameobject_template(entry,type,Data7) VALUES(214862,3,214863),(214863,3,214863),(999999,3,214863);
$migration
SELECT 'first',entry,type,Data7 FROM gameobject_template ORDER BY entry;
$migration
SELECT 'second',entry,type,Data7 FROM gameobject_template ORDER BY entry;
UPDATE gameobject_template SET type=6,Data7=214863 WHERE entry=214862;
$migration
SELECT 'other-type',Data7 FROM gameobject_template WHERE entry=214862;
UPDATE gameobject_template SET type=3,Data7=12345 WHERE entry=214862;
$migration
SELECT 'other-link',Data7 FROM gameobject_template WHERE entry=214862;
"@
   [IO.File]::WriteAllText("$out/guard.sql",$sql);$db=(Get-Content $configPath|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$oldPassword=$env:MYSQL_PWD
   try{$env:MYSQL_PWD=$db[3];$exit=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names') -InputFile "$out/guard.sql" -OutputFile "$out/guard.tsv" -ErrorFile "$out/guard-stderr.log";if($exit -or (Get-Item "$out/guard-stderr.log").Length){throw 'TEMP migration guards failed'}}finally{$env:MYSQL_PWD=$oldPassword}
   $rows=Get-Content "$out/guard.tsv";foreach($label in @('first','second')){foreach($row in @("214862`t3`t0","214863`t3`t214863","999999`t3`t214863")){if($rows -notcontains "$label`t$row"){throw 'Migration replay changed unrelated row/link'}}};if($rows -notcontains "other-type`t214863" -or $rows -notcontains "other-link`t12345"){throw 'Migration altered another type/custom link'};if($VerifyLiveData -and $rows -notcontains "live`t0`t43369`t66863"){throw 'Live target/retained loot/open-text differs'}
  }
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/HatchlingLinkedTrapNative.cpp -ProbeName HatchlingLinkedTrapNative -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw "Native Hatchling loader failed: $out/catalog.log"}
 }}catch{
  if(!$Baseline -or $_.Exception.Message -notlike 'New runtime diagnostics require investigation:*'){throw}
  $d=Get-Content "$out/evidence/diagnostics-after.json" -Raw|ConvertFrom-Json
  if(!$d.Completed -or @($d.ChangedCrashes).Count -or @($d.AppendedDBErrors).Count -ne 1 -or @($d.ServerDiagnostics).Count -ne 1){throw 'Unexpected pinned linked-trap diagnostics'}
  foreach($line in @($d.ServerDiagnostics)+@($d.AppendedDBErrors.Text -split "`n"|Where-Object {$_.Trim()})){if($line -notlike '*Gameobject (Entry: 214862 GoType: 3) have data7=214863 but GO (Entry 214863) have not GAMEOBJECT_TYPE_TRAP (6) type.*'){throw 'Unexplained baseline diagnostic'}}
  'COUNTERFACTUAL: exact warm-loader non-trap error in DBErrors and diagnostic evidence, no other failure; pair repaired ordinary native loader run.'
 }
}finally{$env:LEGION_DIALOGUE_CONFIG=$oldConfig;$env:CL=$oldCompiler}
if((Get-FileHash $configPath).Hash -ne $configHash){throw 'Canonical configuration changed'};foreach($p in $inputs){if((Get-FileHash "$repo/$p").Hash -ne $hashes[$p]){throw 'Native inputs/canonical binary changed during test'}}
@{Inputs=$hashes;Probe=(Get-FileHash "$repo/build-extractors/runtime-test/legion-legendary-catalog/HatchlingLinkedTrapNative.exe").Hash;NativeReleaseNotAsan=$true}|ConvertTo-Json|Set-Content "$out/native-inputs.json"
Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|CASE|COUNTERFACTUAL)'}
