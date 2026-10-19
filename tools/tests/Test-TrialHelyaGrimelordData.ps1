param([string]$AdminUser='root')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/trial-helya-grimelord-data";New-Item -ItemType Directory -Force $out | Out-Null
$connection=[regex]::Match((Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw),'(?m)^WorldDatabaseInfo\s*=\s*"([^"]+)"').Groups[1].Value.Split(';')
function Query([string]$sql,[switch]$Admin){
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=if($Admin){$env:CODEX_MYSQL_ADMIN_PASSWORD}else{$connection[3]};$user=if($Admin){$AdminUser}else{$connection[2]}
  $rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$user --database=$($connection[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE){throw 'Grimelord SQL check failed'};$rows
 }finally{$env:MYSQL_PWD=$previous}
}
$rows=@(Query @'
SELECT 'bindings',COUNT(*) FROM spell_script_names WHERE (spell_id=193367 AND ScriptName='spell_tov_fetid_rot_aura') OR (spell_id=228838 AND ScriptName='spell_tov_fetid_rot_targets');
SELECT 'spread',COUNT(*) FROM spell_script_names WHERE spell_id=228397 AND ScriptName='spell_fetid_rot';
SELECT 'legacy',COUNT(*) FROM spell_aura_trigger WHERE spell_id=193367 AND spell_trigger=228397 AND `option`=1 AND target=0 AND caster=0 AND effectmask=1;
'@)
foreach($expected in @("bindings`t2","spread`t1","legacy`t1")){if($rows -notcontains $expected){throw "Grimelord live data mismatch: $expected"}}
$rows | Set-Content "$out/world.tsv"
$migration=Get-Content "$repo/sql/updates/world/2026_10_19_02_helya_fetid_rot.sql" -Raw
$fixture=@'
CREATE TEMPORARY TABLE codex_rot_shape LIKE spell_script_names;
CREATE TEMPORARY TABLE spell_script_names LIKE codex_rot_shape;
INSERT INTO spell_script_names VALUES(228397,'spell_fetid_rot'),(193367,'unrelated_script');
'@
$result=@(Query ($fixture+"`n"+$migration+"`nSELECT 'first',ROW_COUNT();`n"+$migration+"`nSELECT 'repeat',ROW_COUNT(); SELECT 'preserved',COUNT(*) FROM spell_script_names WHERE (spell_id=228397 AND ScriptName='spell_fetid_rot') OR (spell_id=193367 AND ScriptName='unrelated_script'); SELECT 'total',COUNT(*) FROM spell_script_names;") -Admin)
$result | Set-Content "$out/migration.tsv"
foreach($expected in @("first`t2","repeat`t0","preserved`t2","total`t4")){if($result -notcontains $expected){throw "Scoped Grimelord migration mismatch: $expected"}}
'PASS: scoped Fetid Rot bindings, retained legacy route and exact migration first/repeat preserve unrelated scripts.'
