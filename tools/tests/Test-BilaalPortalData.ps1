param([Parameter(Mandatory=$true)][string]$MigrationPath,[switch]$VerifyLive)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/bilaal-portal-data"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories $out -Test {
 $migration=Get-Content $MigrationPath -Raw
 $native=Get-Content "$PSScriptRoot/BilaalPortalOriginal.json" -Raw|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='entryorguid IN(104546,104902,106747,104502,106708)';creature_template='entry IN(104546,104902,106747)';instance_template='map=1583';waypoints='entry IN(104546,106747,10674700,10674701)';gameobject_template='entry=248553';conditions='SourceTypeOrReferenceId=22 AND SourceEntry IN(104546,104902,106747)'}
 $setup='SET SESSION group_concat_max_len=1048576;'
 foreach($t in $filters.Keys){$setup+="CREATE TEMPORARY TABLE saved_$t LIKE $t;INSERT INTO saved_$t SELECT * FROM $t WHERE $($filters[$t]);CREATE TEMPORARY TABLE $t LIKE saved_$t;INSERT INTO $t SELECT * FROM saved_$t;"}
 $values=@();foreach($row in $native.rows){$cells=@();foreach($field in $native.fields){$v=$row.$field;if($v -is [string]){$cells+="'"+$v.Replace("'","''")+"'"}else{$cells+=[Convert]::ToString($v,[Globalization.CultureInfo]::InvariantCulture)}};$values+='('+($cells -join ',')+')'}
 $restore='DELETE FROM smart_scripts WHERE entryorguid=104546;INSERT INTO smart_scripts('+($native.fields -join ',')+') VALUES'+($values -join ',')+';'
 $setup+=$restore
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$number=0
 function Query([string]$sql,[switch]$Expected){
  $script:number++;$stderr="$out/sql-stderr-$script:number.txt";$prior=$env:MYSQL_PWD;$priorErrors=$ErrorActionPreference
  try{$env:MYSQL_PWD=$db[3];$ErrorActionPreference='Continue';$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names --force 2> $stderr);$code=$LASTEXITCODE}finally{$env:MYSQL_PWD=$prior;$ErrorActionPreference=$priorErrors}
  $err=Get-Content $stderr -Raw
  if($Expected){if(@($err -split "`n"|Where-Object {$_ -match '^ERROR '}).Count -ne 1 -or $err -notmatch '1644 \(45000\)'){throw 'Expected scoped portal refusal missing'}}elseif($code -or $err){throw 'Portal TEMP/read-only query failed'}
  $rows
 }
 function Snapshot([string]$label){$sql='';foreach($t in $filters.Keys){$sql+="SELECT '$label-$t',a.* FROM $t a;"};$sql}
 function Signature($rows,[string]$prefix){(@($rows|Where-Object {$_ -like "$prefix`t*"}|ForEach-Object {$_.Substring($prefix.Length+1)}|Sort-Object -CaseSensitive)) -join "`n"}
 $fields=$native.fields -join ','
 $r=Query "$setup $(Snapshot 'before') SET SESSION group_concat_max_len=32; $migration SELECT 'limit',@@SESSION.group_concat_max_len; $(Snapshot 'first') $migration $(Snapshot 'second') SET SESSION group_concat_max_len=1048576; SELECT 'graph',COUNT(*),SHA2(GROUP_CONCAT(CONCAT_WS('|',$fields) ORDER BY source_type,id SEPARATOR '\n'),256) FROM smart_scripts WHERE entryorguid=104546; $restore $(Snapshot 'normalized')"
 foreach($t in $filters.Keys){if((Signature $r "first-$t") -cne (Signature $r "second-$t")){throw 'Portal migration not idempotent'};if((Signature $r "before-$t") -cne (Signature $r "normalized-$t")){throw "Unrelated portal data changed: $t"}}
 if($r -notcontains "limit`t32" -or $r -notcontains "graph`t11`t83d195ce948f80d71ddbc824ab23a32b5c11fbb6ec7e476202e2701da1fcd571"){throw 'Portal repaired graph/aggregation limit differs'}
 $changes=@("UPDATE creature_template SET ScriptName='custom' WHERE entry=104546;","UPDATE instance_template SET script='custom' WHERE map=1583;",'UPDATE waypoints SET position_x=4000 WHERE entry=104546;','INSERT INTO waypoints(entry,pointid,position_x,position_y,position_z) VALUES(104546,2,1,2,3);','DELETE FROM waypoints WHERE entry=104546;','UPDATE gameobject_template SET Data0=1 WHERE entry=248553;','UPDATE smart_scripts SET event_param1=2 WHERE entryorguid=104546 AND source_type=0 AND id=2;',"INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,comment) VALUES(104546,0,99,40,64,'custom');",'DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=104546 AND ElseGroup=0;','UPDATE conditions SET ConditionTarget=0 WHERE SourceTypeOrReferenceId=22 AND SourceEntry=104546;')
 foreach($applied in @($false,$true)){$prior=if($applied){$migration}else{''};foreach($change in $changes){$r=Query "$setup $prior $change $(Snapshot 'before') $migration $(Snapshot 'after')" -Expected;foreach($t in $filters.Keys){if((Signature $r "before-$t") -cne (Signature $r "after-$t")){throw 'Refused portal migration mutated data'}}}}
 if($VerifyLive){
  $sha=[Security.Cryptography.SHA1]::Create();try{$hash=[BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes(([IO.File]::ReadAllText((Resolve-Path $MigrationPath))).Replace("`r`n","`n")))).Replace('-','')}finally{$sha.Dispose()}
  $r=Query "SET SESSION TRANSACTION READ ONLY;SET SESSION group_concat_max_len=1048576;SELECT 'update',hash,state FROM updates WHERE name='2026_10_27_13_bilaal_portal_arrival.sql';SELECT 'graph',COUNT(*),SHA2(GROUP_CONCAT(CONCAT_WS('|',$fields) ORDER BY source_type,id SEPARATOR '\n'),256) FROM smart_scripts WHERE entryorguid=104546;"
  if($r -notcontains "update`t$hash`tRELEASED" -or $r -notcontains "graph`t11`t83d195ce948f80d71ddbc824ab23a32b5c11fbb6ec7e476202e2701da1fcd571"){throw 'Live portal updater/graph differs'}
 }
 'PASS six actual-schema TEMP tables, full31-field graphs, repeat and low aggregation limit,20 custom refusals; spellclick/timed list/earlier Azuregos/death graph/path/template/conditions preserved. No character or persistent writes.'
}
