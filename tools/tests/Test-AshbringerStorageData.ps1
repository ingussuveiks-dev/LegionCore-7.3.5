param([string]$MigrationPath,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$MigrationPath){$MigrationPath="$repo/sql/updates/world/2026_11_13_02_ashbringer_storage.sql"}
$out="$repo/build-extractors/runtime-test/ashbringer-storage-data/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $migration=Get-Content $MigrationPath -Raw
 $filters=[ordered]@{gameobject_template='entry IN(247318,249824)';gameobject='guid=128117';spell_loot_template='Entry IN(199827,4954)';spell_script_names="spell_id IN(180850,199827) OR ScriptName='spell_bs_ashbringer_storage'"}
 $order=@{gameobject_template='entry';gameobject='guid';spell_loot_template='Entry,Item';spell_script_names='spell_id,ScriptName'}
 $setup='';foreach($table in $filters.Keys){$setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"}
 $setup+="DELETE FROM spell_script_names WHERE spell_id=180850 AND BINARY ScriptName=BINARY 'spell_bs_ashbringer_storage';"
 function Snapshot([string]$tag){$s='';foreach($table in $filters.Keys){$s+="SELECT '$tag-$table',t.* FROM $table t ORDER BY $($order[$table]);"};$s}
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $index=$script:queryIndex++;$inputPath="$out/query-$index.sql";$outputPath="$out/query-$index.tsv";$errorPath="$out/query-$index.stderr.log";[IO.File]::WriteAllText($inputPath,$sql);$prior=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$db[3];$args=@("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names');if($Refusal){$args+='--force'}
   $exit=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $args -InputFile $inputPath -OutputFile $outputPath -ErrorFile $errorPath
   $errors=Get-Content $errorPath -Raw;$rows=@(Get-Content $outputPath)
   if($Refusal){$failures=@($errors -split "`n"|Where-Object {$_ -match '^ERROR '});if($failures.Count -ne 1 -or $failures[0] -notlike '*ERROR 1644 (45000)*Ashbringer storage native source/binding differs*'){throw "Unexpected refusal: $errorPath"}}
   elseif($exit -or $errors.Length){throw "TEMP storage SQL failed: $errorPath"}
   return ,$rows
  }finally{$env:MYSQL_PWD=$prior}
 }
 $live=if($VerifyLiveData){"SELECT 'live',spell_id,ScriptName FROM spell_script_names WHERE spell_id=180850;SELECT 'updater',hash,state FROM updates WHERE name='2026_11_13_02_ashbringer_storage.sql';"}else{''}
 $rows=Query ($live+$setup+(Snapshot 'before')+$migration+(Snapshot 'first')+$migration+(Snapshot 'second'))
 if($VerifyLiveData -and "live`t180850`tspell_bs_ashbringer_storage" -cnotin $rows){throw 'Live paired binding missing'}
 if($VerifyLiveData -and "updater`t$((Get-FileHash $MigrationPath -Algorithm SHA1).Hash)`tRELEASED" -notin $rows){throw 'Actual canonical updater hash/state missing'}
 foreach($table in $filters.Keys){
  if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw 'Storage migration not idempotent'}
  if($table -ne 'spell_script_names' -and (Signature $rows "before-$table") -cne (Signature $rows "first-$table")){throw "Native data unexpectedly changed: $table"}
 }
 # Compare using the generic label stripper, including unrelated mappings.
 $before=(Signature $rows 'before-spell_script_names')
 $first=(Signature $rows 'first-spell_script_names')
 $expected=@(@($before -split "`n"|Where-Object {$_})+"180850`tspell_bs_ashbringer_storage")|Sort-Object
 if((($first -split "`n"|Sort-Object) -join "`n") -cne ($expected -join "`n")){throw 'More than the sole paired binding changed'}
 $variants=[ordered]@{
  'custom-binding'="INSERT INTO spell_script_names VALUES(180850,'custom');"
  'wrong-spell'="INSERT INTO spell_script_names VALUES(1,'spell_bs_ashbringer_storage');"
  'child-script'="INSERT INTO spell_script_names VALUES(199827,'custom');"
  'go-script'="UPDATE gameobject_template SET ScriptName='custom' WHERE entry=247318;"
  'go-type'='UPDATE gameobject_template SET type=0 WHERE entry=247318;'
  'go-loot'='UPDATE gameobject_template SET Data1=1 WHERE entry=247318;'
  'spawn-map'='UPDATE gameobject SET map=1220 WHERE guid=128117;'
  'missing-spawn'='DELETE FROM gameobject WHERE guid=128117;'
  'loot-chance'='UPDATE spell_loot_template SET Chance=99 WHERE Entry=199827;'
  'loot-item'='UPDATE spell_loot_template SET Item=25 WHERE Entry=199827;'
  'loot-count'='UPDATE spell_loot_template SET MaxCount=2 WHERE Entry=199827;'
 }
 foreach($variant in $variants.Keys){$rows=Query ($setup+$variants[$variant]+(Snapshot 'custom')+$migration+(Snapshot 'after')) -Refusal;foreach($table in $filters.Keys){if((Signature $rows "custom-$table") -cne (Signature $rows "after-$table")){throw "Refused $variant state changed: $table"}}}
 "PASS TEMP full-schema storage migration: sole paired180850 binding,idempotent;11 explicit drift refusals including force-continuation;GO/spawn/native loot/unrelated bindings retained;no persistent writes"
}
