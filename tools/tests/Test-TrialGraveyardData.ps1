param([string]$AdminUser='root')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/trial-graveyard-data"
New-Item -ItemType Directory -Force $out | Out-Null
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$connection=[regex]::Match($config,'(?m)^WorldDatabaseInfo\s*=\s*"([^"]+)"').Groups[1].Value.Split(';')
function Query([string]$sql,[switch]$Admin){
 $previous=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=if($Admin){$env:CODEX_MYSQL_ADMIN_PASSWORD}else{$connection[3]}
  $user=if($Admin){$AdminUser}else{$connection[2]}
  $rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$user --database=$($connection[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE){throw 'Trial graveyard SQL check failed'}
  $rows
 }finally{$env:MYSQL_PWD=$previous}
}
$rows=@(Query @'
SELECT 'raid',COUNT(*) FROM game_graveyard_zone WHERE ghost_zone=8440;
SELECT 'external',COUNT(*) FROM game_graveyard_zone WHERE id=5779 AND ghost_zone=8440 AND faction=0;
SELECT 'stormheim',COUNT(*) FROM game_graveyard_zone WHERE id=5779 AND ghost_zone=7541 AND faction=0;
SELECT 'healer',COUNT(*) FROM creature WHERE guid=11253145 AND id=6491 AND map=1220 AND zoneId=7541 AND phaseMask=1 AND PhaseId='' AND POW(position_x-2370.78,2)+POW(position_y-895.832,2)+POW(position_z-252.924,2)<2500;
'@)
foreach($tag in @('raid','external','stormheim','healer')){if($rows -notcontains "$tag`t1"){throw "Trial graveyard data mismatch: $tag"}}
$rows | Set-Content "$out/world.tsv"
# Only temporary tables are mutated by this regression, never live World data.
$migration=Get-Content "$repo/sql/updates/world/2026_10_20_13_trial_of_valor_graveyard.sql" -Raw
$fixture=@'
CREATE TEMPORARY TABLE codex_graveyard_shape LIKE game_graveyard_zone;
CREATE TEMPORARY TABLE game_graveyard_zone LIKE codex_graveyard_shape;
INSERT INTO game_graveyard_zone VALUES(5779,7541,0),(5041,7541,0),(5827,8500,67);
'@
$result=@(Query ($fixture+"`n"+$migration+"`nSELECT 'first',ROW_COUNT();`n"+$migration+"`nSELECT 'repeat',ROW_COUNT();`n"+@'
SELECT 'retained',COUNT(*) FROM game_graveyard_zone WHERE (id=5779 AND ghost_zone=7541 AND faction=0) OR (id=5041 AND ghost_zone=7541 AND faction=0) OR (id=5827 AND ghost_zone=8500 AND faction=67);
UPDATE game_graveyard_zone SET faction=469 WHERE id=5779 AND ghost_zone=8440;
'@+"`n"+$migration+"`nSELECT 'custom',ROW_COUNT(); SELECT 'custom-retained',COUNT(*) FROM game_graveyard_zone WHERE id=5779 AND ghost_zone=8440 AND faction=469;") -Admin)
foreach($expected in @("first`t1","repeat`t0","retained`t3","custom`t0","custom-retained`t1")){if($result -notcontains $expected){throw "Trial migration mismatch: $expected"}}
$result | Set-Content "$out/migration.tsv"
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TrialGraveyardCatalog.cpp -ProbeName TrialGraveyardCatalog -OutputFile "$out/native-catalog.txt"
'PASS: native Trial exterior/checkpoints, neutral raid/Stormheim links and nearby Spirit Healer; exact migration repeat and unrelated/custom preservation.'
