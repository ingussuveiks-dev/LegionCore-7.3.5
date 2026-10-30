param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-farewell-caster"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_03_marksmanship_farewell_caster.sql" -Raw
$reset="UPDATE smart_scripts SET link=3,action_param2=0,target_type=21,target_param1=50 WHERE entryorguid=100749 AND source_type=0 AND id=2;"
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=100749;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
INSERT INTO creature_template SELECT * FROM saved_actor;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=100749 OR action_type=85;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
$reset
UPDATE smart_scripts SET action_param2=128,target_type=130 WHERE entryorguid=100749 AND source_type=0 AND id=2;
SELECT 'before',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;
$reset
$migration
$migration
SELECT 'after',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT 'row',action_type,action_param1,action_param2,target_type,target_param1 FROM smart_scripts WHERE entryorguid=100749 AND source_type=0 AND id=2;
SELECT 'opt-in',COUNT(*) FROM smart_scripts WHERE action_type=85 AND (action_param2 & 128)<>0;
$reset
UPDATE creature_template SET ScriptName='custom_actor';
$migration
SELECT 'custom-script',action_param2 FROM smart_scripts WHERE entryorguid=100749 AND source_type=0 AND id=2;
UPDATE creature_template SET ScriptName='';
$reset
UPDATE smart_scripts SET action_param2=2 WHERE entryorguid=100749 AND source_type=0 AND id=2;
$migration
SELECT 'custom-flags',action_param2 FROM smart_scripts WHERE entryorguid=100749 AND source_type=0 AND id=2;
$reset
UPDATE smart_scripts SET target_param1=100 WHERE entryorguid=100749 AND source_type=0 AND id=2;
$migration
SELECT 'custom-range',action_param2 FROM smart_scripts WHERE entryorguid=100749 AND source_type=0 AND id=2;
$reset
UPDATE smart_scripts SET action_param1=200848 WHERE entryorguid=100749 AND source_type=0 AND id=2;
$migration
SELECT 'custom-spell',action_param2 FROM smart_scripts WHERE entryorguid=100749 AND source_type=0 AND id=2;
SELECT 'approved-owner-cast',entryorguid,source_type,id,action_type,action_param1,action_param2,target_type,target_param1 FROM smart_scripts WHERE action_type=85 AND (action_param2 & 128)<>0 AND NOT(entryorguid=100749 AND source_type=0 AND id=2) ORDER BY entryorguid,source_type,id;
SELECT '__FAREWELL_CASTER_SQL_COMPLETE__';
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];[IO.File]::WriteAllText("$out/fixture.sql",$sql,[Text.UTF8Encoding]::new($false));$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names') -InputFile "$out/fixture.sql" -OutputFile "$out/fixture.stdout" -ErrorFile "$out/fixture.stderr";if($code -ne 0 -or (Get-Item "$out/fixture.stderr").Length){throw 'Caster temporary SQL fixture failed'};$rows=@(Get-Content "$out/fixture.stdout" -Encoding UTF8);if(!$rows.Count -or $rows[-1] -cne '__FAREWELL_CASTER_SQL_COMPLETE__'){throw 'Incomplete native farewell caster fixture'};$rows=@($rows|Select-Object -SkipLast 1)}finally{$env:MYSQL_PWD=$previous}
$expectedOptins=1;$approved=@($rows|Where-Object {$_ -like "approved-owner-cast`t*"})
if($approved.Count){
 $sothozz=@($approved|Where-Object {$_ -ceq "approved-owner-cast`t103226`t0`t7`t85`t204364`t128`t130`t70"})
 $energy=@($approved|Where-Object {$_ -ceq "approved-owner-cast`t104627`t0`t1`t85`t152435`t128`t130`t100" -or $_ -ceq "approved-owner-cast`t106569`t0`t0`t85`t152435`t128`t130`t100"})
 if($sothozz.Count -gt 1 -or $energy.Count -notin @(0,2) -or $approved.Count -ne ($sothozz.Count+$energy.Count)){throw 'Unexpected/partial additional ordinary Player-caster opt-in; review independently'}
 # Admit only the independently released full producer/receiver/story context
 # or complete two-site resource/bar/quest context, never a count-only exception.
 if($sothozz.Count){& "$PSScriptRoot/Test-SothozzInvokerData.ps1" -VerifyLiveData}
 if($energy.Count){& "$PSScriptRoot/Test-ArtifactResourceData.ps1" -VerifyLiveData}
 $expectedOptins=1+$approved.Count
}
foreach($expected in @("row`t85`t200832`t128`t130`t50","opt-in`t$expectedOptins","custom-script`t0","custom-flags`t2","custom-range`t0","custom-spell`t0")){if($rows -notcontains $expected){throw "Caster migration mismatch: $expected"}}
$before=@($rows | Where-Object {$_ -like "before`t*"} | ForEach-Object {$_.Substring(7)})
$after=@($rows | Where-Object {$_ -like "after`t*"} | ForEach-Object {$_.Substring(6)})
if(Compare-Object $before $after){throw 'Unrelated cast/death fields changed'}
[IO.File]::WriteAllLines("$out/row.tsv",[string[]]@($rows | Where-Object {$_ -like "row`t*"} | ForEach-Object {$_.Substring(4)}))
& "$PSScriptRoot/Test-ScriptPetCredit.ps1" -InvokerCastRowsPath "$out/row.tsv" -VerifyReleaseBinary:$VerifyReleaseBinary
if($VerifyReleaseBinary){$old=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_smart_player_cast_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Player caster PE regression failed'}}finally{$env:PYTHONPATH=$old}}
'PASS: scoped Torvis85/200832 opt-in preserves every other native cast/death field and custom definitions, twice-idempotent'
