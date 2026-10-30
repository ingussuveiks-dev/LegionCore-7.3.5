param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/blood-scene-equip-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_08_blood_scene_equip.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_08_blood_scene_equip.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql+"`nSELECT '__BLOOD_SCENE_SQL_COMPLETE__';`n")
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];$sqlFile="$out/sql-$($script:queryIndex-1).sql";$stdoutFile="$out/sql-$($script:queryIndex-1).stdout"
   # Preserve complete native stdout/stderr bytes, including the tail after an
   # expected --force error; a PowerShell native pipeline lost part of a row.
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile $sqlFile -OutputFile $stdoutFile -ErrorFile $errorFile
   $rows=@(Get-Content $stdoutFile -Encoding UTF8);$errors=[string](Get-Content $errorFile -Raw -Encoding UTF8)
   if($Refusal){if($errors -notmatch 'Blood native scenes/equip/admission differ; review before applying' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Blood scene equip refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Blood scene equip SQL failed: $errorFile"}
   if(!$rows.Count -or $rows[-1] -cne '__BLOOD_SCENE_SQL_COMPLETE__'){throw "Incomplete native SQL output: $stdoutFile"}
   return ,@($rows|Select-Object -SkipLast 1)
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/BloodSceneEquipOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{spell_scene='MiscValue=1168';spell_scene_event='MiscValue=1168';conditions='SourceTypeOrReferenceId=17 AND SourceEntry IN(200134,205138)';gameobject_template='entry=247566'}
 $order=@{spell_scene='MiscValue,SceneScriptPackageID';spell_scene_event='MiscValue,Event';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2';gameobject_template='entry'}
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
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Not idempotent $table"};if($table -eq 'gameobject_template' -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Unrelated table changed $table"}}
 $oldScenes=@((Signature $rows 'original-spell_scene').Split("`n"));$newScenes=@((Signature $rows 'first-spell_scene').Split("`n"))
 if($oldScenes.Count -ne 1 -or $newScenes.Count -ne 1){throw 'Expected one native scene record'}
 for($i=0;$i -lt 1;++$i){$f=$oldScenes[$i].Split("`t");if($f[4] -ne 'NULL'){throw 'Original scene already bound'};$f[4]='scene_blood_artifact_equipment';if(($f -join "`t") -cne $newScenes[$i]){throw 'Native scene field changed beyond scoped script binding'}}
 if((Signature $rows 'original-spell_scene_event') -or (Signature $rows 'original-conditions')){throw 'Expected pinned original missing callbacks/cast rules'}
 $events=@((Signature $rows 'first-spell_scene_event').Split("`n")|ForEach-Object {($_.Split("`t")[0..3]) -join "`t"})
 if(($events -join ',') -cne "1168`tcomplete`t200134`t0"){throw 'Expected exact native callbacks'}
 $conditions=@((Signature $rows 'first-conditions').Split("`n"));if($conditions.Count -ne 33){throw 'Expected33 exact scoped predicates'}
 foreach($path in @(,@(200134,1168,40740,1489,940,9,128402))){$spell=$path[0];$selected=@($conditions|Where-Object {[int]$_.Split("`t")[2] -eq $spell})
  if($selected.Count -ne 33){throw 'Expected33 rules per native spell'}
  foreach($g in 0..4){$v=@($selected|Where-Object {[int]$_.Split("`t")[4] -eq $g})
   $expected=if($g -eq 0){,@(22,$path[3],0,0,1)}else{$state=if(($g%2) -eq 1){9}else{28};$stage=$path[5]+[Math]::Floor(($g-1)/2);@(@(2,$path[6],1,0,0),@(15,32,0,0,0),@(22,$path[3],0,0,0),@(44,$path[4],$stage,0,0),@($state,$path[2],0,0,0),@(8,$path[2],0,0,1),@(36,0,0,0,0),@(40,$path[1],0,0,0))}
   $expected=if($g -eq 0){@("22,$($path[3]),0,0,1")}else{@($expected|ForEach-Object {$_ -join ','})};$actual=@($v|ForEach-Object {$f=$_.Split("`t");if(($f[0,1,3,6,11,12] -join ',') -ne '17,0,0,0,0,'){throw 'Unexpected source/target/script'};($f[5,7,8,9,10]) -join ','})
   if($v.Count -ne $(if($g -eq 0){1}else{8}) -or @(Compare-Object $expected $actual -CaseSensitive).Count){throw "Unexpected real Blood cast group $spell/$g"}
  }
  [IO.File]::WriteAllLines("$out/rules-$spell.tsv",[string[]]@($selected|ForEach-Object {($_.Split("`t")[4..10]) -join "`t"}))
 }
 [IO.File]::WriteAllLines("$out/events.tsv",[string[]]$events);[IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'package'='UPDATE spell_scene SET SceneScriptPackageID=1 WHERE MiscValue=1168;';
 'flags'='UPDATE spell_scene SET PlaybackFlags=0 WHERE MiscValue=1168;';
 'duration'='UPDATE spell_scene SET CustomDuration=1000 WHERE MiscValue=1168;';
 'script'="UPDATE spell_scene SET ScriptName='custom' WHERE MiscValue=1168;";
 'event'="INSERT INTO spell_scene_event VALUES(1168,'custom',1,0,'custom');";
 'condition'="INSERT INTO conditions VALUES(17,0,200134,0,0,4,0,1,0,0,0,0,'','custom');";
 'alternate'="INSERT INTO conditions VALUES(17,0,205138,0,0,4,0,1,0,0,0,0,'','custom');";
 'loot-dependency'='UPDATE gameobject_template SET Data1=0 WHERE entry=247566;';
 'partial-events'="INSERT INTO spell_scene_event VALUES(1168,'complete',200134,0,'Blood scene completion equips owned artifact');";
 'applied-alternate-spell'='UPDATE spell_scene_event SET trigerSpell=205138 WHERE MiscValue=1168;';
 'applied-foreign-spell'='UPDATE spell_scene_event SET trigerSpell=196020 WHERE MiscValue=1168;';
 'applied-credit'='UPDATE spell_scene_event SET MonsterCredit=109440 WHERE MiscValue=1168;';
 'applied-extra-cancel'="INSERT INTO spell_scene_event VALUES(1168,'cancel',200134,0,'custom');";
 'applied-missing-event'='DELETE FROM spell_scene_event WHERE MiscValue=1168;';
 'applied-wrong-scene'='UPDATE conditions SET ConditionValue1=1282 WHERE ConditionTypeOrReference=40;';
 'applied-outside'='UPDATE conditions SET NegativeCondition=0 WHERE ConditionTypeOrReference=22 AND ElseGroup=0;';
 'applied-bank'='UPDATE conditions SET ConditionValue3=1 WHERE ConditionTypeOrReference=2;';
 'applied-quest'='UPDATE conditions SET ConditionValue1=39247 WHERE ConditionTypeOrReference=9;';
 'applied-stage'='UPDATE conditions SET ConditionValue2=5 WHERE SourceEntry=200134 AND ConditionTypeOrReference=44;';
 'applied-class'='UPDATE conditions SET ConditionValue1=1024 WHERE ConditionTypeOrReference=15;';
 'applied-missing-predicate'='DELETE FROM conditions WHERE SourceEntry=200134 AND ElseGroup=1 AND ConditionTypeOrReference=40;';
 'applied-same-count'="DELETE FROM conditions WHERE SourceEntry=200134 AND ElseGroup=1 AND ConditionTypeOrReference=40;INSERT INTO conditions VALUES(17,0,200134,0,1,4,0,1,0,0,0,0,'','custom');"
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state changed $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_08_blood_scene_equip.sql' AND hash='$hash' AND state='RELEASED';"
  if($live -notcontains '1'){throw 'Live Blood scene equip wave migration/hash differs'}
  $fullLive=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $fullLive "live-$table") -cne (Signature $rows "first-$table")){throw "Live differs from Blood scene equip regression $table"}}
 }
 'PASS full-schema TEMP Blood real scene callback:1 scoped script binding/1 complete mapping/33 exact cast predicates; native packages/flags/loot/alternate spell unchanged, idempotence/22 full custom-partial refusals/live equality'
}
