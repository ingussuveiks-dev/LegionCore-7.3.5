param([string]$MigrationPath="$PSScriptRoot/../../sql/updates/world/2026_10_27_09_bilaal_instance_death.sql",[switch]$VerifyLive)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/bilaal-death-data"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories $out -Test {
 $migration=Get-Content $MigrationPath -Raw
 $native=Get-Content "$PSScriptRoot/BilaalDeathOriginal.json" -Raw|ConvertFrom-Json
 $tables=@('smart_scripts','creature_template','creature','instance_template','conditions','creature_summon_groups')
 $filters=@{
 smart_scripts='entryorguid IN(104502,106708,104546,104902,106747,-370522,-370551) OR(action_type=12 AND action_param1 IN(104502,106708))'
 creature_template='entry IN(104502,106708,104546,104902,106747)'
 creature='id IN(104502,106708,104546)'
 instance_template='map IN(1583,1545)'
 conditions='SourceTypeOrReferenceId=22 AND SourceEntry IN(104502,106708,104546,104902,106747,-370522,-370551)'
 creature_summon_groups='entry IN(104502,106708,104546)'
 }
 $setup='SET SESSION group_concat_max_len=1048576;'
 foreach($table in $tables){$setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;"}
 $values=@();foreach($row in $native.rows){$cell=@();foreach($field in $native.fields){$v=$row.$field;if($v -is [string]){$cell+="'"+$v.Replace("'","''")+"'"}else{$cell+=[Convert]::ToString($v,[Globalization.CultureInfo]::InvariantCulture)}};$values+='('+($cell -join ',')+')'}
 $insert='INSERT INTO smart_scripts('+($native.fields -join ',')+') VALUES'+($values -join ',')+';'
 $setup+='DELETE FROM smart_scripts WHERE entryorguid IN(104502,106708) AND source_type=0;'+$insert
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $queryNumber=0
 function Query([string]$sql,[switch]$Expected){
  $script:queryNumber++;$errPath="$out/sql-stderr-$script:queryNumber.txt";$prior=$env:MYSQL_PWD
  $priorErrors=$ErrorActionPreference
  try{$env:MYSQL_PWD=$db[3];$ErrorActionPreference='Continue';$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names --force 2> $errPath);$code=$LASTEXITCODE}finally{$env:MYSQL_PWD=$prior;$ErrorActionPreference=$priorErrors}
  $err=Get-Content $errPath -Raw
  if($Expected){if($err -notmatch '1644 \(45000\)' -or @($err -split "`n"|Where-Object {$_ -match '^ERROR '}).Count -ne 1){throw 'Expected scoped Bilaal death refusal missing'}}elseif($code -or $err){throw 'Bilaal death TEMP/read-only query failed'}
  $rows
 }
 function Snapshot([string]$label){$sql='';foreach($t in $tables){$sql+="SELECT '$label-$t',a.* FROM $t a;"};$sql}
 function Signature($rows,[string]$label){(@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)}|Sort-Object -CaseSensitive)) -join "`n"}
 $restore='DELETE FROM smart_scripts WHERE entryorguid IN(104502,106708) AND source_type=0;'+$insert
 $rows=Query "$setup`n$(Snapshot 'original')`nSET SESSION group_concat_max_len=32;`n$migration`nSELECT 'restored',@@SESSION.group_concat_max_len;`n$(Snapshot 'first')`n$migration`n$(Snapshot 'second')`nSELECT 'fixed',COUNT(*) FROM smart_scripts WHERE entryorguid IN(104502,106708) AND source_type=0; $restore $(Snapshot 'normalized')"
 if($rows -notcontains "restored`t32"){throw 'Migration did not preserve caller aggregation limit'}
 foreach($t in $tables){if((Signature $rows "first-$t") -cne (Signature $rows "second-$t")){throw 'Bilaal death migration not idempotent'};if((Signature $rows "original-$t") -cne (Signature $rows "normalized-$t")){throw "Bilaal death migration changed an unrelated field/table: $t"}}
 if($rows -notcontains "fixed`t10"){throw 'Native combat rows were not retained exactly'}
 $changes=@(
 "UPDATE creature_template SET ScriptName='custom' WHERE entry=104502;",
 "UPDATE creature_template SET AIName='' WHERE entry=104546;",
 "UPDATE instance_template SET script='custom' WHERE map=1583;",
 'UPDATE instance_template SET parent=1 WHERE map=1583;',
 "UPDATE creature SET PhaseId='6068' WHERE guid=370522;",
 'DELETE FROM creature WHERE guid=370551;',
 'INSERT INTO creature(guid,id,map,PhaseId,phaseMask) VALUES(9999999,104502,1583,6140,1);',
 'DELETE FROM smart_scripts WHERE entryorguid=104502 AND source_type=0 AND id=2;',
 'UPDATE smart_scripts SET event_param2=81 WHERE entryorguid=106708 AND source_type=0 AND id=2;',
 "INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,action_param1,comment) VALUES(104502,0,99,6,205,92,'custom');",
 'INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,6,104502,1,123);',
 'INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,6,-370522,1,123);',
 "INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,action_param1,comment) VALUES(-370551,0,0,6,205,92,'custom');",
 "INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,action_param1,comment) VALUES(9999999,0,0,6,12,104502,'custom');",
 'INSERT INTO creature_summon_groups(summonerId,summonerType,groupId,entry) VALUES(9999999,0,0,106708);'
 )
 foreach($applied in @($false,$true)){
  $prior=if($applied){$migration}else{''}
  foreach($change in $changes){
   $r=Query "$setup`n$prior`n$change`n$(Snapshot 'before')`n$migration`n$(Snapshot 'after')" -Expected
   foreach($t in $tables){if((Signature $r "before-$t") -cne (Signature $r "after-$t")){throw "Refused Bilaal death migration changed $t"}}
  }
 }
 if($VerifyLive){
  $sha=[Security.Cryptography.SHA1]::Create();try{$hash=[BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes(([IO.File]::ReadAllText((Resolve-Path $MigrationPath))).Replace("`r`n","`n")))).Replace('-','')}finally{$sha.Dispose()}
  $fields=$native.fields -join ','
  $r=Query "SET SESSION TRANSACTION READ ONLY;SET SESSION group_concat_max_len=1048576;SELECT 'update',hash,state FROM updates WHERE name='2026_10_27_09_bilaal_instance_death.sql';SELECT 'fixed',COUNT(*),SHA2(GROUP_CONCAT(CONCAT_WS('|',$fields) ORDER BY entryorguid,id SEPARATOR '\n'),256) FROM smart_scripts WHERE entryorguid IN(104502,106708) AND source_type=0;"
  if($r -notcontains "update`t$hash`tRELEASED" -or $r -notcontains "fixed`t10`tc7162d84ebc8a7f4fd2cc65690dac77cb35518c5bf40981bac3a165a873c1c49"){throw 'Bilaal live death updater/hash/full graphs differ'}
 }
 'PASS six TEMP tables, full31-field original/applied graphs, repeat application, native combat/earlier Azuregos/template/phase preservation and30 custom refusals. No character mutation.'
}
