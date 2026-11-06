param([string]$MigrationPath,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$MigrationPath){$MigrationPath="$repo/sql/pending/world/2026_11_06_01_slaghammer_companion_owner.sql";if(!(Test-Path $MigrationPath)){$MigrationPath="$repo/sql/updates/world/2026_11_06_01_slaghammer_companion_owner.sql"}}
$out="$repo/build-extractors/runtime-test/slaghammer-companion-data/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $migration=Get-Content $MigrationPath -Raw
 $fixture=Get-Content "$PSScriptRoot/SlaghammerCompanionOriginal.json" -Raw|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='entryorguid IN(101899,101430,103151) AND source_type IN(0,9)';conditions='SourceTypeOrReferenceId=22 AND SourceEntry IN(101899,101430,103151)';creature_template='entry IN(101899,101430,103151)';spell_script_names='spell_id=201004'}
 $order=@{smart_scripts='entryorguid,source_type,id';conditions='SourceTypeOrReferenceId,SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference';creature_template='entry';spell_script_names='spell_id,ScriptName'}
 function Value($v){if($null -eq $v){'NULL'}elseif($v -is [string]){if($v.Length){'CONVERT(0x'+([BitConverter]::ToString([Text.Encoding]::UTF8.GetBytes($v))).Replace('-','')+' USING utf8mb4)'}else{"''"}}else{[Convert]::ToString($v,[Globalization.CultureInfo]::InvariantCulture)}}
 $setup='';foreach($table in $filters.Keys){
  $setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"
  if($table -eq 'creature_template'){continue} # Preserve the full live template, including required TEXT columns.
  $scope=switch($table){'smart_scripts'{'entryorguid=101899 AND source_type=0'}'conditions'{'SourceTypeOrReferenceId=22 AND SourceEntry=101899'}'creature_template'{'entry=101899'}'spell_script_names'{'spell_id=201004'}}
  $setup+="DELETE FROM $table WHERE $scope;`n"
  foreach($row in $fixture.$table){$cols=($row.PSObject.Properties.Name|ForEach-Object {"``$_``"}) -join ',';$values=($row.PSObject.Properties|ForEach-Object {Value $_.Value}) -join ',';$setup+="INSERT INTO $table($cols) VALUES($values);`n"}
 }
 $setup+="INSERT INTO smart_scripts(entryorguid,source_type,id,comment) VALUES(999999,0,0,'preserve foreign actor'),(101899,9,99,'preserve timed list');INSERT INTO spell_script_names VALUES(999999,'preserve_foreign_binding');`n"
 function Snapshot([string]$tag){$s='';foreach($table in $filters.Keys){$s+="SELECT '$tag-$table',t.* FROM $table t ORDER BY $($order[$table]);"};$s}
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $index=$script:queryIndex++;$inputPath="$out/query-$index.sql";$outputPath="$out/query-$index.tsv";$errorPath="$out/query-$index.stderr.log"
  [IO.File]::WriteAllText($inputPath,$sql);$prior=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$db[3];$args=@("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names');if($Refusal){$args+='--force'}
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $args -InputFile $inputPath -OutputFile $outputPath -ErrorFile $errorPath
   $errors=Get-Content $errorPath -Raw;$rows=@(Get-Content $outputPath)
   if($Refusal){$failures=@($errors -split "`n"|Where-Object {$_ -match '^ERROR '});if($failures.Count -ne 1 -or $failures[0] -notlike '*ERROR 1644 (45000)*Slaghammer companion source/conditions/binding differ*'){throw "Unexpected explicit refusal diagnostics: $errorPath"}}
   elseif($code -or $errors.Length){throw "TEMP companion SQL failed: $errorPath"}
   return ,$rows
  }finally{$env:MYSQL_PWD=$prior}
 }
 $live=if($VerifyLiveData){"SELECT 'live',target_type,target_param1 FROM smart_scripts WHERE entryorguid=101899 AND source_type=0 AND id=1;"}else{''}
 $rows=Query ($live+$setup+(Snapshot 'before')+$migration+(Snapshot 'first')+$migration+(Snapshot 'second'))
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw 'Companion migration not idempotent'};if($table -ne 'smart_scripts' -and (Signature $rows "before-$table") -cne (Signature $rows "first-$table")){throw "Unrelated data changed: $table"}}
 $before=@($rows|Where-Object {$_ -like "before-smart_scripts`t*"}|ForEach-Object {$_.Substring(21)});$after=@($rows|Where-Object {$_ -like "first-smart_scripts`t*"}|ForEach-Object {$_.Substring(20)})
 if($before.Count -ne $after.Count){throw 'Companion migration changed row count'};$changed=0
 for($i=0;$i -lt $before.Count;++$i){if($before[$i] -cne $after[$i]){$old=$before[$i].Split("`t");$new=$after[$i].Split("`t");if($old.Count -ne 31 -or $old[0] -ne '101899' -or $old[1] -ne '0' -or $old[2] -ne '1' -or $old[21] -ne '21' -or $old[22] -ne '100' -or $new[21] -ne '23' -or $new[22] -ne '0'){throw 'Unexpected changed companion row'};$old[21]='23';$old[22]='0';if(($old -join "`t") -cne $after[$i]){throw 'More than the two intended cells changed'};++$changed}}
 if($changed -ne 1){throw 'Expected exactly one retargeted follow row'}
 $variants=[ordered]@{
  'distance'='UPDATE smart_scripts SET target_param1=99 WHERE entryorguid=101899 AND source_type=0 AND id=1;'
  'target'='UPDATE smart_scripts SET target_type=7 WHERE entryorguid=101899 AND source_type=0 AND id=1;'
  'event'='UPDATE smart_scripts SET event_type=10 WHERE entryorguid=101899 AND source_type=0 AND id=1;'
  'spell'='UPDATE smart_scripts SET action_param1=202894 WHERE entryorguid=101899 AND source_type=0 AND id=9;'
  'link'='UPDATE smart_scripts SET link=0 WHERE entryorguid=101899 AND source_type=0 AND id=2;'
  'credit'='UPDATE smart_scripts SET action_param2=48307 WHERE entryorguid=101899 AND source_type=0 AND id=13;'
  'condition'='UPDATE conditions SET ConditionValue2=4 WHERE SourceEntry=101899 AND SourceGroup=2;'
  'extra-row'="INSERT INTO smart_scripts(entryorguid,source_type,id,comment) VALUES(101899,0,99,'custom');"
  'ai'="UPDATE creature_template SET AIName='custom' WHERE entry=101899;"
  'script'="UPDATE creature_template SET ScriptName='custom' WHERE entry=101899;"
  'binding'="INSERT INTO spell_script_names VALUES(201004,'custom');"
  'applied-partial'='UPDATE smart_scripts SET target_type=23 WHERE entryorguid=101899 AND source_type=0 AND id=1;'
 }
 foreach($variant in $variants.Keys){$rows=Query ($setup+$variants[$variant]+(Snapshot 'custom')+$migration+(Snapshot 'after')) -Refusal;foreach($table in $filters.Keys){if((Signature $rows "custom-$table") -cne (Signature $rows "after-$table")){throw "Custom $variant state changed: $table"}}}
 if($VerifyLiveData -and (Query $live) -notcontains "live`t23`t0"){throw 'Live companion follow not deployed'}
 'PASS TEMP-only complete-schema migration twice: exactly2follow cells changed;14 actions/2stage gates,shared101430/103151 LINK2 and foreign actor/timed list/binding retained;12 explicit custom/partial refusals preserved all tables. Live readback='+$VerifyLiveData
}
