param([string]$MigrationPath,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$MigrationPath){$MigrationPath="$repo/sql/pending/world/2026_11_15_69_slaghammer_link2_combat.sql";if(!(Test-Path $MigrationPath)){$MigrationPath="$repo/sql/updates/world/2026_11_15_69_slaghammer_link2_combat.sql"}}
$out="$repo/build-extractors/runtime-test/slaghammer-link2-data/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $migration=Get-Content $MigrationPath -Raw
 $filters=[ordered]@{smart_scripts='entryorguid IN(101122,101430,101899,103151) AND source_type IN(0,9)';conditions='SourceTypeOrReferenceId=22 AND SourceEntry IN(101122,101430,101899,103151)';creature_template='entry IN(101122,101430,101899,103151)';instance_template='map=1539';waypoints='entry=101430';spell_script_names='spell_id IN(200165,201004)'}
 $order=@{smart_scripts='entryorguid,source_type,id';conditions='SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference';creature_template='entry';instance_template='map';waypoints='entry,pointid';spell_script_names='spell_id,ScriptName'}
 $setup='';foreach($table in $filters.Keys){$setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"}

 # Replay starts from the historical absent node, in TEMP tables only. The
 # actual applied node/conditions are separately certified by migration replay
 # and live readback; foreign/custom rows are never normalized away.
 $setup+=$migration; $setup+="DELETE FROM smart_scripts WHERE @slaghammer_link2_safe AND entryorguid=101430 AND source_type=0 AND id=2;DELETE FROM conditions WHERE @slaghammer_link2_safe AND SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=101430;"
 function Snapshot([string]$tag){$s='';foreach($table in $filters.Keys){$s+="SELECT '$tag-$table',t.* FROM $table t ORDER BY $($order[$table]);"};$s}
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $index=$script:queryIndex++;$inputPath="$out/query-$index.sql";$outputPath="$out/query-$index.tsv";$errorPath="$out/query-$index.stderr.log";[IO.File]::WriteAllText($inputPath,$sql);$prior=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$db[3];$args=@("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names');if($Refusal){$args+='--force'}
   $exit=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $args -InputFile $inputPath -OutputFile $outputPath -ErrorFile $errorPath
   $errors=Get-Content $errorPath -Raw;$rows=@(Get-Content $outputPath)
   if($Refusal){$failures=@($errors -split "`n"|Where-Object {$_ -match '^ERROR '});if($failures.Count -ne 1 -or $failures[0] -notlike '*ERROR 1644 (45000)*Slaghammer LINK2 graph/binding/conditions differ*'){throw "Unexpected refusal: $errorPath"}}
   elseif($exit -or $errors.Length){throw "TEMP handoff SQL failed: $errorPath"}
   return ,$rows
  }finally{$env:MYSQL_PWD=$prior}
 }
 $live=if($VerifyLiveData){"SELECT 'live',AIName,ScriptName FROM creature_template WHERE entry=101430;SELECT 'updater',hash,state FROM updates WHERE name='2026_11_15_69_slaghammer_link2_combat.sql';"}else{''}
 $sha1=[Security.Cryptography.SHA1]::Create();try{$migrationHash=([BitConverter]::ToString($sha1.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha1.Dispose()}; $rows=Query ($live+"SELECT 'column',COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='creature_template' ORDER BY ORDINAL_POSITION;"+$setup+(Snapshot 'before')+$migration+(Snapshot 'first')+$migration+(Snapshot 'second'))
 if($VerifyLiveData -and "live`tSmartAI`tnpc_slaghammer_handoff" -cnotin $rows){throw 'Live derived SmartAI binding missing'}
 if($VerifyLiveData -and "updater`t$migrationHash`tRELEASED" -notin $rows){throw 'Actual canonical updater hash/state missing'}
 foreach($table in $filters.Keys){
  if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw 'LINK2 migration not idempotent'}
  $before=@((Signature $rows "before-$table") -split "`n");$after=@((Signature $rows "first-$table") -split "`n")
  if($table -eq 'smart_scripts'){
   $extra=@($after|Where-Object {$_ -match '^101430\t0\t2\t'});if($extra.Count -ne 1 -or $extra[0] -notmatch '\t49\t0\t0\t0\t0\t0\t0\t19\t101122\t50\t'){throw 'Actual reconstructed native AttackStart row differs'}
   $after=@($after|Where-Object {$_ -notmatch '^101430\t0\t2\t'})
  }
  if($table -eq 'conditions'){
   $extra=@($after|Where-Object {$_ -match '^22\t3\t101430\t'});if($extra.Count -ne 8){throw 'Eight source22 combat admission rows missing'}
   $after=@($after|Where-Object {$_ -notmatch '^22\t3\t101430\t'})
  }
  if(($before -join "`n") -cne ($after -join "`n")){throw "Unrelated native $table changed"}
 }
 $variants=[ordered]@{
  'custom-binding'="UPDATE creature_template SET ScriptName='custom' WHERE entry=101430;"
  'custom-ai'="UPDATE creature_template SET AIName='custom' WHERE entry=101430;"
  'native-cast'='UPDATE smart_scripts SET action_param1=201005 WHERE entryorguid=101430 AND source_type=0 AND id=9;'
  'unknown-link2'="INSERT INTO smart_scripts(entryorguid,source_type,id,comment) VALUES(101430,0,2,'custom unknown action');"
  'condition'='UPDATE conditions SET ConditionValue1=1 WHERE SourceTypeOrReferenceId=22 AND SourceEntry=101430 AND SourceGroup=1 AND ConditionTypeOrReference=9;'
  'combat-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1,Comment) VALUES(22,3,101430,31,3,'custom');"
  'instance'="UPDATE instance_template SET script='custom' WHERE map=1539;"
  'custom-guard'="UPDATE creature_template SET faction=35 WHERE entry=101122;"
 }
 foreach($variant in $variants.Keys){$rows=Query ($setup+$variants[$variant]+(Snapshot 'custom')+$migration+(Snapshot 'after')) -Refusal;foreach($table in $filters.Keys){if((Signature $rows "custom-$table") -cne (Signature $rows "after-$table")){throw "Refused $variant state changed: $table"}}}
 "PASS TEMP full-schema reconstructed LINK2: one native AttackStart row/eight invoker gates, replay and8 drift refusals; original other scripts/conditions/actors retained,103151 unknown link remains; no persistent writes"
}
