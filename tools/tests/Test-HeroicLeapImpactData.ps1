param([string]$MigrationPath="$PSScriptRoot/../../sql/updates/world/2026_10_27_11_heroic_leap_landing.sql",[switch]$VerifyLive)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/heroic-leap-impact-data"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories $out -Test {
 $migration=Get-Content $MigrationPath -Raw
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $number=0
 function Query([string]$sql,[switch]$Expected){
  $script:number++;$stderr="$out/sql-stderr-$script:number.txt";$prior=$env:MYSQL_PWD;$priorErrors=$ErrorActionPreference
  try{$env:MYSQL_PWD=$db[3];$ErrorActionPreference='Continue';$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names --force 2> $stderr);$code=$LASTEXITCODE}finally{$env:MYSQL_PWD=$prior;$ErrorActionPreference=$priorErrors}
  $err=Get-Content $stderr -Raw
  if($Expected){if(@($err -split "`n"|Where-Object {$_ -match '^ERROR '}).Count -ne 1 -or $err -notmatch '1644 \(45000\)'){throw 'Expected scoped Leap refusal missing'}}elseif($code -or $err){throw 'Leap TEMP/read-only query failed'}
  $rows
 }
 $setup=@'
CREATE TEMPORARY TABLE leap_script_shape LIKE spell_script_names;
CREATE TEMPORARY TABLE spell_script_names LIKE leap_script_shape;
INSERT INTO spell_script_names VALUES(6544,'spell_warr_heroic_leap'),(100,'unrelated');
CREATE TEMPORARY TABLE leap_delay_shape LIKE spell_trigger_delay;
CREATE TEMPORARY TABLE spell_trigger_delay LIKE leap_delay_shape;
INSERT INTO spell_trigger_delay VALUES(6544,52174,0,0,0,7,0,0,'original'),(100,123,0,0,0,7,0,0,'unrelated');
CREATE TEMPORARY TABLE spell_linked_spell AS SELECT * FROM spell_linked_spell WHERE spell_trigger IN(6544,52174);
'@
 function Snapshot([string]$label){"SELECT '$label-s',s.* FROM spell_script_names s ORDER BY spell_id,ScriptName;SELECT '$label-d',d.* FROM spell_trigger_delay d ORDER BY spell_id,spell_trigger,target;SELECT '$label-l',l.* FROM spell_linked_spell l ORDER BY spell_trigger,spell_effect;"}
 function Signature($rows,[string]$prefix){(@($rows|Where-Object {$_ -like "$prefix`t*"}|ForEach-Object {$_.Substring($prefix.Length+1)})) -join "`n"}
 $rows=Query "$setup $(Snapshot 'before') $migration $(Snapshot 'first') $migration $(Snapshot 'second') SELECT 'arrival',spell_id,spell_trigger,effectmask FROM spell_trigger_delay WHERE spell_id=94954; DELETE FROM spell_script_names WHERE spell_id=94954;DELETE FROM spell_trigger_delay WHERE spell_id=94954;$(Snapshot 'normalized')"
 foreach($table in @('s','d','l')){
  if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw 'Leap migration not idempotent'}
  if((Signature $rows "before-$table") -cne (Signature $rows "normalized-$table")){throw 'Leap migration changed original/unrelated/talent rows'}
 }
 if($rows -notcontains "arrival`t94954`t52174`t2"){throw 'Native actual-jump arrival missing'}
 $changes=@("INSERT INTO spell_script_names VALUES(94954,'custom');","DELETE FROM spell_script_names WHERE spell_id=6544;","INSERT INTO spell_script_names VALUES(6544,'custom');",'DELETE FROM spell_trigger_delay WHERE spell_id=6544;','UPDATE spell_trigger_delay SET effectmask=4 WHERE spell_id=6544;','INSERT INTO spell_trigger_delay VALUES(94954,123,0,0,0,2,0,0,NULL);','INSERT INTO spell_trigger_delay VALUES(94954,52174,1,0,0,2,0,0,NULL);')
 foreach($applied in @($false,$true)){
  $prior=if($applied){$migration}else{''}
  foreach($change in $changes){$r=Query "$setup $prior $change $(Snapshot 'before') $migration $(Snapshot 'after')" -Expected;foreach($t in @('s','d','l')){if((Signature $r "before-$t") -cne (Signature $r "after-$t")){throw 'Refused Leap migration mutated data'}}}
 }
 if($VerifyLive){
  $sha=[Security.Cryptography.SHA1]::Create();try{$hash=[BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes(([IO.File]::ReadAllText((Resolve-Path $MigrationPath))).Replace("`r`n","`n")))).Replace('-','')}finally{$sha.Dispose()}
  $r=Query "SET SESSION TRANSACTION READ ONLY;SELECT 'script',ScriptName FROM spell_script_names WHERE spell_id=94954;SELECT 'arrival',spell_trigger,effectmask FROM spell_trigger_delay WHERE spell_id=94954;SELECT 'talent',spell_effect,type,hastalent FROM spell_linked_spell WHERE spell_trigger=52174 AND spell_effect IN(202164,199085,209484);SELECT 'update',hash,state FROM updates WHERE name='2026_10_27_11_heroic_leap_landing.sql';"
  foreach($row in @("script`tspell_warr_heroic_leap_jump","arrival`t52174`t2","talent`t202164`t8`t202163","talent`t199085`t3`t199086","talent`t209484`t8`t209483","update`t$hash`tRELEASED")){if($r -notcontains $row){throw "Live Leap contract differs: $row"}}
 }
 'PASS actual-schema TEMP/repeat/original/talent preservation and14 custom refusals; no character writes'
}
