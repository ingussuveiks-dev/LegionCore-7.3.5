param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/slaghammer-intro-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_29_17_slaghammer_intro_admission.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_29_17_slaghammer_intro_admission.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql)
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];[Console]::OutputEncoding=[Text.UTF8Encoding]::new($false);$OutputEncoding=[Text.UTF8Encoding]::new($false);$ErrorActionPreference='Continue';$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --default-character-set=utf8mb4 --batch --raw --skip-column-names --force 2>$errorFile);$code=$LASTEXITCODE;$ErrorActionPreference='Stop';$errors=[string](Get-Content $errorFile -Raw)
   [IO.File]::WriteAllLines("$out/sql-$($script:queryIndex-1).stdout",[string[]]$rows)
   if($Refusal){if($errors -notmatch 'Slaghammer intro/source/conversation differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Slaghammer refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Slaghammer SQL failed: $errorFile"};return ,$rows
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/SlaghammerIntroOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='entryorguid IN(101430,101122) AND source_type=0';conditions='(SourceTypeOrReferenceId=22 AND SourceEntry IN(101430,101122)) OR (SourceTypeOrReferenceId=15 AND SourceGroup=19199)';creature_template='entry IN(101430,101122)';conversation_data='entry=1031';conversation_actor='entry=1031';spell_script_names='spell_id=200165'}
 $order=@{smart_scripts='entryorguid,source_type,id';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference';creature_template='entry';conversation_data='entry,id';conversation_actor='entry,id';spell_script_names='spell_id,ScriptName'}
 function Value($v){if($null -eq $v){'NULL'}elseif($v -is [string]){if($v.Length){"CONVERT(0x"+([BitConverter]::ToString([Text.Encoding]::UTF8.GetBytes($v))).Replace('-','')+" USING utf8mb4)"}else{"''"}}else{[Convert]::ToString($v,[Globalization.CultureInfo]::InvariantCulture)}}
 $setup='';$baseline=''
 foreach($table in $filters.Keys){
  $setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;`n"
  $baseline+="DELETE FROM $table;`n"
  foreach($row in $fixture.$table){$cols=($row.PSObject.Properties.Name|ForEach-Object {"``$_``"}) -join ',';$values=($row.PSObject.Properties|ForEach-Object {Value $_.Value}) -join ',';$baseline+="INSERT INTO $table($cols) VALUES($values);`n"}
 }
 function Snapshot([string]$tag){$sql='';foreach($table in $filters.Keys){$sql+="SELECT '$tag-$table',t.* FROM $table t WHERE $($filters[$table]) ORDER BY $($order[$table]);"};$sql}
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
 $rows=Query ($setup+$baseline+(Snapshot 'original')+$migration+(Snapshot 'first')+$migration+(Snapshot 'second'))
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Not idempotent $table"};if($table -ne 'conditions' -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Unrelated table changed $table"}}
 $original=@($rows|Where-Object {$_ -like "original-smart_scripts`t*"}|ForEach-Object {$_.Substring(23)})
 $repaired=@($rows|Where-Object {$_ -like "first-smart_scripts`t*"}|ForEach-Object {$_.Substring(20)})
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original);[IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$repaired)
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 if($original.Count -ne 12 -or $repaired.Count -ne 12 -or ($original -join "`n") -cne ($repaired -join "`n")){throw 'Slaghammer/cultist actions or missingLINK2 changed'}
 $oldConditions=@($rows|Where-Object {$_ -like "original-conditions`t*"}|ForEach-Object {$_.Substring(20)})
 $newConditions=@($rows|Where-Object {$_ -like "first-conditions`t*"}|ForEach-Object {$_.Substring(17)})
 if($newConditions.Count -ne $oldConditions.Count+12 -or @($oldConditions|Where-Object {$_ -cnotin $newConditions}).Count){throw 'Existing conditions changed or expected12 new guards missing'}
 [IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]$newConditions)
 $variants=[ordered]@{
  'ai'="UPDATE creature_template SET AIName='custom' WHERE entry=101430;";
  'criterion'='UPDATE smart_scripts SET action_param2=48160 WHERE entryorguid=101430 AND id=0;';
  'spell'='UPDATE smart_scripts SET action_param1=201004 WHERE entryorguid=101430 AND id=1;';
  'missing-link'='UPDATE smart_scripts SET link=0 WHERE entryorguid=101430 AND id=1;';
  'cultist-guard'='UPDATE conditions SET ConditionValue2=3 WHERE SourceEntry=101122;';
  'conversation-actor'='UPDATE conversation_actor SET creatureId=101899 WHERE id=1;';
  'conversation-timing'='UPDATE conversation_data SET textId=0 WHERE id=2523;';
  'binding'="INSERT INTO spell_script_names VALUES(200165,'custom');";
  'condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,1,101430,9,1);";
  'extra-row'="INSERT INTO smart_scripts(entryorguid,source_type,id,comment) VALUES(101430,0,99,'custom');";
  'applied-target'='UPDATE conditions SET ConditionTarget=0 WHERE SourceEntry=101430 AND SourceGroup=1 AND ConditionTypeOrReference=44;';
  'applied-link-stage'='UPDATE conditions SET ConditionValue2=3 WHERE SourceEntry=101430 AND SourceGroup=2 AND ElseGroup=1 AND ConditionTypeOrReference=44;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state changed $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_29_17_slaghammer_intro_admission.sql' AND hash='$hash' AND state='RELEASED';"
  if($live -notcontains '1'){throw 'Live Slaghammer migration/hash differs'}
  $fullLive=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $fullLive "live-$table") -cne (Signature $rows "first-$table")){throw "Live differs from Slaghammer regression $table"}}
 }
 'PASS full-schema TEMP Slaghammer intro replay/idempotence:12 scoped conditions only; all12 actions including unknown LINK2, existing cultist guard/native conversation/actors/bindings retained; twelve custom-state refusals'
}
