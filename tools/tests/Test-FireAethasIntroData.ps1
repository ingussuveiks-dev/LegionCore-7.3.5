param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/fire-aethas-intro-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_16_fire_aethas_intro.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_16_fire_aethas_intro.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql+"`nSELECT '__FIRE_AETHAS_INTRO_SQL_COMPLETE__';`n")
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];$sqlFile="$out/sql-$($script:queryIndex-1).sql";$stdoutFile="$out/sql-$($script:queryIndex-1).stdout"
   # Preserve complete native stdout/stderr bytes, including the tail after an
   # expected --force error; a PowerShell native pipeline lost part of a row.
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile $sqlFile -OutputFile $stdoutFile -ErrorFile $errorFile
   $rows=@(Get-Content $stdoutFile -Encoding UTF8);$errors=[string](Get-Content $errorFile -Raw -Encoding UTF8)
   if($Refusal){if($errors -notmatch 'Fire Aethas intro/native return dependencies differ; review before applying' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Fire Aethas intro refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Fire Aethas intro SQL failed: $errorFile"}
   if(!$rows.Count -or $rows[-1] -cne '__FIRE_AETHAS_INTRO_SQL_COMPLETE__'){throw "Incomplete native SQL output: $stdoutFile"}
   return ,@($rows|Select-Object -SkipLast 1)
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/FireAethasIntroOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{conditions='SourceTypeOrReferenceId=22 AND SourceEntry=99763 AND SourceId=0';smart_scripts='source_type=0 AND entryorguid=99763';creature_template='entry IN(99763,99861)';waypoints='entry=99763';npc_spellclick_spells='npc_entry=99861'}
 $order=@{conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2';smart_scripts='entryorguid,source_type,id,link';creature_template='entry';waypoints='entry,pointid';npc_spellclick_spells='npc_entry,spell_id'}
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
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Not idempotent $table"};if($table -ne 'conditions' -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Native intro/return changed $table"}}
 if(Signature $rows 'original-conditions'){throw 'Pinned Aethas original already conditioned'}
 $rules=@((Signature $rows 'first-conditions').Split("`n"));if($rules.Count -ne 2){throw 'Expected two intro predicates'}
 $actual=@($rules|ForEach-Object {$f=$_.Split("`t");if(($f[0..4]+$f[6]+$f[9..12] -join ',') -cne '22,1,99763,0,0,1,0,0,0,'){throw 'Wrong real actor condition source/target/flags'};($f[5,7,8]) -join ','})
 if(@(Compare-Object @('22,1480,0','44,957,3') $actual -CaseSensitive).Count){throw 'Wrong actual native map/active-boss stage'}
 [IO.File]::WriteAllLines("$out/rules.tsv",[string[]]@($rules|ForEach-Object {($_.Split("`t")[4..10]) -join "`t"}))
 [IO.File]::WriteAllLines("$out/actors.tsv",[string[]]((Signature $rows 'first-smart_scripts').Split("`n")))
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'custom-condition'="INSERT INTO conditions VALUES(22,1,99763,0,0,4,1,1,0,0,0,0,'','custom');";
 'intro-spell'='UPDATE smart_scripts SET action_param1=1 WHERE id=1;';
 'intro-link'='UPDATE smart_scripts SET link=0 WHERE id=0;';
 'death-cleanup'='UPDATE smart_scripts SET action_type=1 WHERE id=4;';
 'return-portal'='UPDATE smart_scripts SET action_param1=1 WHERE id=9;';
 'return-conversation'='UPDATE smart_scripts SET action_param1=1 WHERE id=10;';
 'waypoint'='UPDATE waypoints SET position_z=0 WHERE pointid=1;';
 'portal-click'='UPDATE npc_spellclick_spells SET spell_id=1;';
 'actor-ai'="UPDATE creature_template SET ScriptName='custom' WHERE entry=99763;";
 'partial'="INSERT INTO conditions VALUES(22,1,99763,0,0,22,1,1480,0,0,0,0,'','Fire Aethas battle intro: actual actor map and active boss step');";
 'applied-target'='UPDATE conditions SET ConditionTarget=0;';
 'applied-map'='UPDATE conditions SET ConditionValue1=1489 WHERE ConditionTypeOrReference=22;';
 'applied-scenario'='UPDATE conditions SET ConditionValue1=901 WHERE ConditionTypeOrReference=44;';
 'applied-stage'='UPDATE conditions SET ConditionValue2=4 WHERE ConditionTypeOrReference=44;';
 'applied-root'='UPDATE conditions SET SourceGroup=5;';
 'applied-missing'='DELETE FROM conditions WHERE ConditionTypeOrReference=44;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state mutated $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_16_fire_aethas_intro.sql' AND hash='$hash' AND state='RELEASED';";if($live -notcontains '1'){throw 'Live Aethas intro migration/hash differs'}
  $live=Query (Snapshot 'live')
  $live=& "$PSScriptRoot/Convert-VerifiedFireAethasTemplate.ps1" -Rows $live -Fixture $fixture
  foreach($table in $filters.Keys){if((Signature $live "live-$table") -cne (Signature $rows "first-$table")){throw "Live Aethas intro/native return differs $table"}}
 }
 'PASS full5table native Aethas intro: exactly2 source22 actor-target1 predicates for1480/957/3; original actor actions/data3/waypoint/portal/click unchanged, idempotence/16 custom-partial refusals/live equality'
}
