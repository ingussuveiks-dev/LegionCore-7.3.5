param([switch]$Apply,[string]$AdminUser='root')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/trial-helya-mariner-data";New-Item -ItemType Directory -Force $out | Out-Null
$c=[regex]::Match((Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw),'(?m)^WorldDatabaseInfo\s*=\s*"([^"]+)"').Groups[1].Value.Split(';')
function Query([string]$sql,[switch]$Admin){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=if($Admin){$env:CODEX_MYSQL_ADMIN_PASSWORD}else{$c[3]};$user=if($Admin){$AdminUser}else{$c[2]};$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$user --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Mariner data query failed'};$rows}finally{$env:MYSQL_PWD=$previous}}
$migration=Get-Content "$repo/sql/updates/world/2026_10_20_00_helya_give_no_quarter.sql" -Raw
$fixture=@'
CREATE TEMPORARY TABLE codex_mariner_shape LIKE spell_script_names;
CREATE TEMPORARY TABLE spell_script_names LIKE codex_mariner_shape;
INSERT INTO spell_script_names VALUES(228383,'spell_tov_darkness'),(228633,'unrelated_script');
'@
$replay=@(Query ($fixture+"`n"+$migration+"`nSELECT 'first',ROW_COUNT();`n"+$migration+"`nSELECT 'repeat',ROW_COUNT();SELECT 'preserved',COUNT(*) FROM spell_script_names WHERE (spell_id=228383 AND ScriptName='spell_tov_darkness') OR (spell_id=228633 AND ScriptName='unrelated_script');SELECT 'total',COUNT(*) FROM spell_script_names;") -Admin)
foreach($expected in @("first`t1","repeat`t0","preserved`t2","total`t3")){if($replay -notcontains $expected){throw "Mariner migration replay mismatch: $expected"}}
$replay | Set-Content "$out/migration.tsv"
if($Apply){Query $migration -Admin | Out-Null}
$rows=@(Query "SELECT 'binding',COUNT(*) FROM spell_script_names WHERE spell_id=228633 AND ScriptName='spell_tov_give_no_quarter';SELECT 'darkness',COUNT(*) FROM spell_script_names WHERE spell_id=228383 AND ScriptName='spell_tov_darkness';")
foreach($expected in @("binding`t1","darkness`t1")){if($rows -notcontains $expected){throw "Mariner live binding mismatch: $expected"}}
$rows | Set-Content "$out/world.tsv"
'PASS: exact Mariner migration replay is idempotent, preserves unrelated scripts and matches live selector/Darkness bindings.'
