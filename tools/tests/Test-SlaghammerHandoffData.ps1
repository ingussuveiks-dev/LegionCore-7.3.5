param([string]$MigrationPath,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$MigrationPath){$MigrationPath="$repo/sql/pending/world/2026_11_06_03_slaghammer_handoff.sql";if(!(Test-Path $MigrationPath)){$MigrationPath="$repo/sql/updates/world/2026_11_06_03_slaghammer_handoff.sql"}}
$out="$repo/build-extractors/runtime-test/slaghammer-handoff-data/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $migration=Get-Content $MigrationPath -Raw
 $filters=[ordered]@{smart_scripts='entryorguid IN(101122,101430,101899,103151) AND source_type IN(0,9)';conditions='SourceTypeOrReferenceId=22 AND SourceEntry IN(101122,101430,101899,103151)';creature_template='entry IN(101122,101430,101899,103151)';instance_template='map=1539';waypoints='entry=101430';spell_script_names='spell_id IN(200165,201004)'}
 $order=@{smart_scripts='entryorguid,source_type,id';conditions='SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference';creature_template='entry';instance_template='map';waypoints='entry,pointid';spell_script_names='spell_id,ScriptName'}
 $setup='';foreach($table in $filters.Keys){$setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"}
 $setup+="UPDATE creature_template SET ScriptName='' WHERE entry=101430 AND ScriptName='npc_slaghammer_handoff';"
 function Snapshot([string]$tag){$s='';foreach($table in $filters.Keys){$s+="SELECT '$tag-$table',t.* FROM $table t ORDER BY $($order[$table]);"};$s}
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $index=$script:queryIndex++;$inputPath="$out/query-$index.sql";$outputPath="$out/query-$index.tsv";$errorPath="$out/query-$index.stderr.log";[IO.File]::WriteAllText($inputPath,$sql);$prior=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$db[3];$args=@("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names');if($Refusal){$args+='--force'}
   $exit=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $args -InputFile $inputPath -OutputFile $outputPath -ErrorFile $errorPath
   $errors=Get-Content $errorPath -Raw;$rows=@(Get-Content $outputPath)
   if($Refusal){$failures=@($errors -split "`n"|Where-Object {$_ -match '^ERROR '});if($failures.Count -ne 1 -or $failures[0] -notlike '*ERROR 1644 (45000)*Slaghammer handoff native graph/binding/path differ*'){throw "Unexpected refusal: $errorPath"}}
   elseif($exit -or $errors.Length){throw "TEMP handoff SQL failed: $errorPath"}
   return ,$rows
  }finally{$env:MYSQL_PWD=$prior}
 }
 $live=if($VerifyLiveData){"SELECT 'live',AIName,ScriptName FROM creature_template WHERE entry=101430;SELECT 'updater',hash,state FROM updates WHERE name='2026_11_06_03_slaghammer_handoff.sql';"}else{''}
 $rows=Query ($live+"SELECT 'column',COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='creature_template' ORDER BY ORDINAL_POSITION;"+$setup+(Snapshot 'before')+$migration+(Snapshot 'first')+$migration+(Snapshot 'second'))
 if($VerifyLiveData -and "live`tSmartAI`tnpc_slaghammer_handoff" -cnotin $rows){throw 'Live derived SmartAI binding missing'}
 if($VerifyLiveData -and "updater`t$((Get-FileHash $MigrationPath -Algorithm SHA1).Hash)`tRELEASED" -notin $rows){throw 'Actual canonical updater hash/state missing'}
 foreach($table in $filters.Keys){
  if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw 'Handoff migration not idempotent'}
  if($table -ne 'creature_template' -and (Signature $rows "before-$table") -cne (Signature $rows "first-$table")){throw "Native data unexpectedly changed: $table"}
 }
 $columns=@($rows|Where-Object {$_ -like "column`t*"}|ForEach-Object {$_.Substring(7)});$index=[array]::IndexOf($columns,'ScriptName');if($index -lt 0){throw 'Template schema not identified'}
 $before=(Signature $rows 'before-creature_template') -split "`n";$after=(Signature $rows 'first-creature_template') -split "`n";$changes=0
 for($n=0;$n -lt $before.Count;++$n){if($before[$n] -cne $after[$n]){$fields=$before[$n].Split("`t");if($fields[0] -ne '101430' -or $fields[$index] -ne ''){throw 'Unexpected template change'};$fields[$index]='npc_slaghammer_handoff';if(($fields -join "`t") -cne $after[$n]){throw 'More than the binding changed'};++$changes}}
 if($changes -ne 1){throw 'Expected exactly one ScriptName cell change'}
 $variants=[ordered]@{
  'custom-binding'="UPDATE creature_template SET ScriptName='custom' WHERE entry=101430;"
  'custom-ai'="UPDATE creature_template SET AIName='custom' WHERE entry=101430;"
  'native-cast'='UPDATE smart_scripts SET action_param1=201005 WHERE entryorguid=101430 AND source_type=0 AND id=9;'
  'unknown-link2'="INSERT INTO smart_scripts(entryorguid,source_type,id,comment) VALUES(101430,0,2,'custom unknown action');"
  'condition'='UPDATE conditions SET ConditionValue1=1 WHERE SourceTypeOrReferenceId=22 AND SourceEntry=101430 AND SourceGroup=1 AND ConditionTypeOrReference=9;'
  'path'='UPDATE waypoints SET position_x=2021 WHERE entry=101430;'
  'instance'="UPDATE instance_template SET script='custom' WHERE map=1539;"
  'spell-binding'="INSERT INTO spell_script_names VALUES(201004,'custom');"
 }
 foreach($variant in $variants.Keys){$rows=Query ($setup+$variants[$variant]+(Snapshot 'custom')+$migration+(Snapshot 'after')) -Refusal;foreach($table in $filters.Keys){if((Signature $rows "custom-$table") -cne (Signature $rows "after-$table")){throw "Refused $variant state changed: $table"}}}
 "PASS TEMP full-schema handoff migration: one ScriptName cell,idempotent;8 explicit drift refusals;native101430/101899/103151 scripts/conditions and both missingLINK2 retained;no persistent writes"
}
