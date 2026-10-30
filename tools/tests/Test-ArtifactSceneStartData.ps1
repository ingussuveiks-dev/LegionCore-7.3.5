param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-scene-start-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_14_artifact_scene_start.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_14_artifact_scene_start.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql+"`nSELECT '__ARTIFACT_START_SQL_COMPLETE__';`n")
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];$sqlFile="$out/sql-$($script:queryIndex-1).sql";$stdoutFile="$out/sql-$($script:queryIndex-1).stdout"
   # Preserve complete native stdout/stderr bytes, including the tail after an
   # expected --force error; a PowerShell native pipeline lost part of a row.
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile $sqlFile -OutputFile $stdoutFile -ErrorFile $errorFile
   $rows=@(Get-Content $stdoutFile -Encoding UTF8);$errors=[string](Get-Content $errorFile -Raw -Encoding UTF8)
   if($Refusal){if($errors -notmatch 'Artifact scene start dependencies/admission differ; review before applying' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Artifact scene start refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Artifact scene start SQL failed: $errorFile"}
   if(!$rows.Count -or $rows[-1] -cne '__ARTIFACT_START_SQL_COMPLETE__'){throw "Incomplete native SQL output: $stdoutFile"}
   return ,@($rows|Select-Object -SkipLast 1)
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/ArtifactSceneStartOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{conditions='(SourceTypeOrReferenceId=17 AND SourceEntry IN(190435,196023,200152,199502,217814)) OR (SourceTypeOrReferenceId=22 AND SourceId=1 AND SourceEntry IN(247386,247494,247566,248086,251605))';smart_scripts='source_type=1 AND entryorguid IN(247386,247494,247566,248086,251605)';gameobject_template='entry IN(247386,247494,247566,248086,251605)';spell_scene='MiscValue IN(1071,1125,1168,1160,1342)'}
 $order=@{conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2';smart_scripts='entryorguid,source_type,id,link';gameobject_template='entry';spell_scene='MiscValue,SceneScriptPackageID'}
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
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Not idempotent $table"};if($table -ne 'conditions' -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Native dependency changed $table"}}
 $original=@((Signature $rows 'original-conditions').Split("`n"));$conditions=@((Signature $rows 'first-conditions').Split("`n"))
 if(@($original|Where-Object {$_ -like "17`t*"}).Count){throw 'Pinned original already has start rules'}
 $retained=@($conditions|Where-Object {$_ -notlike "17`t*"});if(($retained -join "`n") -cne ($original -join "`n")){throw 'Existing source22 rules changed'}
 $starts=@($conditions|Where-Object {$_ -like "17`t*"});if($starts.Count -ne 140){throw 'Expected140 new start rules'}
 foreach($path in @(@(190435,38990,128292,32,1480,901,6),@(196023,11997,128820,128,1480,957,4),@(200152,40740,128402,32,1489,940,9),@(199502,40570,128940,512,1528,983,6),@(217814,42765,128938,512,1625,1137,7))){
  $spell=$path[0];$selected=@($starts|Where-Object {[int]$_.Split("`t")[2] -eq $spell});if($selected.Count -ne 28){throw 'Expected28 rules per spell'}
  foreach($g in 1..4){$v=@($selected|Where-Object {[int]$_.Split("`t")[4] -eq $g});$state=if($g%2){9}else{28};$stage=$path[6]+[Math]::Floor(($g-1)/2)
   $expected=@(@(2,$path[2],1,0,0),@(15,$path[3],0,0,0),@(22,$path[4],0,0,0),@(44,$path[5],$stage,0,0),@($state,$path[1],0,0,0),@(8,$path[1],0,0,1),@(36,0,0,0,0))|ForEach-Object {$_ -join ','}
   $actual=@($v|ForEach-Object {$f=$_.Split("`t");if(($f[0,1,3,6,11,12] -join ',') -ne '17,0,0,0,0,'){throw 'Unexpected source/target/script'};($f[5,7,8,9,10]) -join ','})
   if($v.Count -ne 7 -or @(Compare-Object $expected $actual -CaseSensitive).Count){throw "Unexpected start predicate $spell/$g"}
  }
  [IO.File]::WriteAllLines("$out/rules-$spell.tsv",[string[]]@($selected|ForEach-Object {($_.Split("`t")[4..10]) -join "`t"}))
 }
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'existing-start'="INSERT INTO conditions VALUES(17,0,196023,0,0,4,0,1,0,0,0,0,'','custom');";
 'existing-root'='UPDATE conditions SET ConditionValue1=1 WHERE SourceTypeOrReferenceId=22 AND ConditionTypeOrReference=15;';
 'native-loot'='UPDATE gameobject_template SET Data1=0 WHERE entry=247494;';
 'native-action'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=247494 AND id=0;';
 'native-triggered'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=247494 AND id=0;';
 'native-link'='UPDATE smart_scripts SET link=9 WHERE entryorguid=248086 AND id=2;';
 'native-scene'="UPDATE spell_scene SET ScriptName='custom' WHERE MiscValue=1125;";
 'native-package'='UPDATE spell_scene SET SceneScriptPackageID=1 WHERE MiscValue=1168;';
 'applied-missing'='DELETE FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=196023 AND ElseGroup=1 AND ConditionTypeOrReference=36;';
 'applied-bank'='UPDATE conditions SET ConditionValue3=1 WHERE SourceTypeOrReferenceId=17 AND ConditionTypeOrReference=2;';
 'applied-class'='UPDATE conditions SET ConditionValue1=1 WHERE SourceTypeOrReferenceId=17 AND SourceEntry=199502 AND ConditionTypeOrReference=15;';
 'applied-quest'='UPDATE conditions SET ConditionValue1=1 WHERE SourceTypeOrReferenceId=17 AND SourceEntry=217814 AND ConditionTypeOrReference=9;';
 'applied-stage'='UPDATE conditions SET ConditionValue2=0 WHERE SourceTypeOrReferenceId=17 AND SourceEntry=190435 AND ConditionTypeOrReference=44;';
 'applied-outside'="INSERT INTO conditions VALUES(17,0,200152,0,0,22,0,1489,0,0,1,0,'','custom');";
 'applied-partial'='DELETE FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=200152;';
 'applied-source-group'='UPDATE conditions SET SourceGroup=1 WHERE SourceTypeOrReferenceId=17 AND SourceEntry=196023;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom data modified $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_14_artifact_scene_start.sql' AND hash='$hash' AND state='RELEASED';";if($live -notcontains '1'){throw 'Live start migration/hash differs'}
  $live=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $live "live-$table") -cne (Signature $rows "first-$table")){throw "Live start dependency differs $table"}}
 }
 'PASS five artifact scene starts:140 exact source17 predicates, existing source22/GO/SmartAI/scene data unchanged, full-schema idempotence/16 custom-partial refusals/live equality; actual delayed cast bridge verified separately'
}
