param([string]$AdminUser='root')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/trial-helya-pool-data";New-Item -ItemType Directory -Force $out | Out-Null
$connection=[regex]::Match((Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw),'(?m)^WorldDatabaseInfo\s*=\s*"([^"]+)"').Groups[1].Value.Split(';')
function Query([string]$sql,[switch]$Admin){
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=if($Admin){$env:CODEX_MYSQL_ADMIN_PASSWORD}else{$connection[3]};$user=if($Admin){$AdminUser}else{$connection[2]}
  $rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$user --database=$($connection[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE){throw 'Bilewater pool SQL check failed'};$rows
 }finally{$env:MYSQL_PWD=$previous}
}
$rows=@(Query @'
SELECT 'binding',COUNT(*) FROM areatrigger_scripts WHERE entry=12892 AND ScriptName='at_bilewater_corrosion';
SELECT 'template',COUNT(*) FROM areatrigger_template WHERE entry=8823 AND customEntry=12892 AND spellId=227992 AND Radius=5;
SELECT 'scope',COUNT(*) FROM areatrigger_template WHERE customEntry=12892;
SELECT 'actions',COUNT(*) FROM areatrigger_actions WHERE entry=8823 AND spellId=227998 AND ((moment=1 AND actionType=0) OR (moment IN(2,8) AND actionType=1));
SELECT 'fury',COUNT(*) FROM areatrigger_scripts WHERE entry=12897 AND ScriptName='at_fury_of_the_maw';
'@)
foreach($tag in @('binding','template','scope','fury')){if($rows -notcontains "$tag`t1"){throw "Bilewater data mismatch: $tag"}}
if($rows -notcontains "actions`t3"){throw 'Bilewater native pool enter/leave/despawn actions changed'}
$rows | Set-Content "$out/world.tsv"
$migration=Get-Content "$repo/sql/updates/world/2026_10_19_01_helya_bilewater_pool_scale.sql" -Raw
$fixture=@'
CREATE TEMPORARY TABLE codex_pool_shape LIKE areatrigger_scripts;
CREATE TEMPORARY TABLE areatrigger_scripts LIKE codex_pool_shape;
INSERT INTO areatrigger_scripts VALUES(12897,'at_fury_of_the_maw');
'@
$result=@(Query ($fixture+"`n"+$migration+"`nSELECT 'first',ROW_COUNT();`n"+$migration+"`nSELECT 'repeat',ROW_COUNT();`nUPDATE areatrigger_scripts SET ScriptName='old' WHERE entry=12892;`n"+$migration+"`nSELECT 'repair',ROW_COUNT(); SELECT 'unrelated',COUNT(*) FROM areatrigger_scripts WHERE entry=12897 AND ScriptName='at_fury_of_the_maw'; SELECT 'total',COUNT(*) FROM areatrigger_scripts;") -Admin)
foreach($expected in @("first`t1","repeat`t0","repair`t2","unrelated`t1","total`t2")){if($result -notcontains $expected){throw "Scoped Bilewater migration mismatch: $expected"}}
$result | Set-Content "$out/migration.tsv"
'PASS: scoped Bilewater binding/template/native actions; exact temporary-table migration first/repeat/repair preserves Fury.'
