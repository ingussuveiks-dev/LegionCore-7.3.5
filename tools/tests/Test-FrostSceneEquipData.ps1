param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/frost-scene-equip-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_11_frost_scene_equip.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_11_frost_scene_equip.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql+"`nSELECT '__FROST_SCENE_SQL_COMPLETE__';`n")
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];$sqlFile="$out/sql-$($script:queryIndex-1).sql";$stdoutFile="$out/sql-$($script:queryIndex-1).stdout"
   # Preserve complete native stdout/stderr bytes, including the tail after an
   # expected --force error; a PowerShell native pipeline lost part of a row.
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile $sqlFile -OutputFile $stdoutFile -ErrorFile $errorFile
   $rows=@(Get-Content $stdoutFile -Encoding UTF8);$errors=[string](Get-Content $errorFile -Raw -Encoding UTF8)
   if($Refusal){if($errors -notmatch 'Frost native scenes/equip/admission differ; review before applying' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Frost scene equip refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Frost scene equip SQL failed: $errorFile"}
   if(!$rows.Count -or $rows[-1] -cne '__FROST_SCENE_SQL_COMPLETE__'){throw "Incomplete native SQL output: $stdoutFile"}
   return ,@($rows|Select-Object -SkipLast 1)
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/FrostSceneEquipOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{spell_scene='MiscValue=1071';spell_scene_event='MiscValue=1071';conditions='SourceTypeOrReferenceId=17 AND SourceEntry IN(197092,186209,186202)';gameobject_template='entry=247386';conversation_data='entry=750';conversation_actor='entry=750';quest_objectives='QuestID=38990'}
 $order=@{spell_scene='MiscValue,SceneScriptPackageID';spell_scene_event='MiscValue,Event';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2';gameobject_template='entry';conversation_data='entry,idx,id';conversation_actor='entry,id';quest_objectives='ID'}
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
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Not idempotent $table"};if($table -in @('gameobject_template','conversation_data','conversation_actor','quest_objectives') -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Unrelated table changed $table"}}
 $oldScenes=@((Signature $rows 'original-spell_scene').Split("`n"));$newScenes=@((Signature $rows 'first-spell_scene').Split("`n"))
 if($oldScenes.Count -ne 1 -or $newScenes.Count -ne 1){throw 'Expected one native scene record'}
 for($i=0;$i -lt 1;++$i){$f=$oldScenes[$i].Split("`t");if($f[4] -ne 'NULL'){throw 'Original scene already bound'};$f[4]='scene_frost_artifact_equipment';if(($f -join "`t") -cne $newScenes[$i]){throw 'Native scene field changed beyond scoped script binding'}}
 if((Signature $rows 'original-spell_scene_event') -or (Signature $rows 'original-conditions')){throw 'Expected pinned original missing callbacks/cast rules'}
 $events=@((Signature $rows 'first-spell_scene_event').Split("`n")|ForEach-Object {($_.Split("`t")[0..3]) -join "`t"})
 if(($events -join ',') -cne "1071`tcomplete`t197092`t0"){throw 'Expected exact native callbacks'}
 $conditions=@((Signature $rows 'first-conditions').Split("`n"));if($conditions.Count -ne 33){throw 'Expected33 exact scoped predicates'}
 foreach($path in @(,@(197092,1071,38990,1480,901,6,128292))){$spell=$path[0];$selected=@($conditions|Where-Object {[int]$_.Split("`t")[2] -eq $spell})
  if($selected.Count -ne 33){throw 'Expected33 rules per native spell'}
  foreach($g in 0..4){$v=@($selected|Where-Object {[int]$_.Split("`t")[4] -eq $g})
   $expected=if($g -eq 0){,@(22,$path[3],0,0,1)}else{$state=if(($g%2) -eq 1){9}else{28};$stage=$path[5]+[Math]::Floor(($g-1)/2);@(@(2,$path[6],1,0,0),@(15,32,0,0,0),@(22,$path[3],0,0,0),@(44,$path[4],$stage,0,0),@($state,$path[2],0,0,0),@(8,$path[2],0,0,1),@(36,0,0,0,0),@(40,$path[1],0,0,0))}
   $expected=if($g -eq 0){@("22,$($path[3]),0,0,1")}else{@($expected|ForEach-Object {$_ -join ','})};$actual=@($v|ForEach-Object {$f=$_.Split("`t");if(($f[0,1,3,6,11,12] -join ',') -ne '17,0,0,0,0,'){throw 'Unexpected source/target/script'};($f[5,7,8,9,10]) -join ','})
   if($v.Count -ne $(if($g -eq 0){1}else{8}) -or @(Compare-Object $expected $actual -CaseSensitive).Count){throw "Unexpected real Frost cast group $spell/$g"}
  }
  [IO.File]::WriteAllLines("$out/rules-$spell.tsv",[string[]]@($selected|ForEach-Object {($_.Split("`t")[4..10]) -join "`t"}))
 }
 [IO.File]::WriteAllLines("$out/events.tsv",[string[]]$events);[IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'package'='UPDATE spell_scene SET SceneScriptPackageID=1 WHERE MiscValue=1071;';
 'flags'='UPDATE spell_scene SET PlaybackFlags=0 WHERE MiscValue=1071;';
 'duration'='UPDATE spell_scene SET CustomDuration=1000 WHERE MiscValue=1071;';
 'script'="UPDATE spell_scene SET ScriptName='custom' WHERE MiscValue=1071;";
 'event'="INSERT INTO spell_scene_event VALUES(1071,'custom',1,0,'custom');";
 'condition'="INSERT INTO conditions VALUES(17,0,197092,0,0,4,0,1,0,0,0,0,'','custom');";
 'alternate'="INSERT INTO conditions VALUES(17,0,186209,0,0,4,0,1,0,0,0,0,'','custom');";
 'conversation-line'='UPDATE conversation_data SET id=1 WHERE entry=750;';
 'conversation-actor'='UPDATE conversation_actor SET duration=1 WHERE entry=750;';
 'pickup-objective'='UPDATE quest_objectives SET ObjectID=1 WHERE QuestID=38990 AND ObjectID=94390;';
 'loot-dependency'='UPDATE gameobject_template SET Data1=0 WHERE entry=247386;';
 'partial-events'="INSERT INTO spell_scene_event VALUES(1071,'complete',197092,0,'Frost scene completion equips owned artifact');";
 'applied-alternate-spell'='UPDATE spell_scene_event SET trigerSpell=186209 WHERE MiscValue=1071;';
 'applied-foreign-spell'='UPDATE spell_scene_event SET trigerSpell=196020 WHERE MiscValue=1071;';
 'applied-credit'='UPDATE spell_scene_event SET MonsterCredit=109440 WHERE MiscValue=1071;';
 'applied-extra-cancel'="INSERT INTO spell_scene_event VALUES(1071,'cancel',197092,0,'custom');";
 'applied-missing-event'='DELETE FROM spell_scene_event WHERE MiscValue=1071;';
 'applied-wrong-scene'='UPDATE conditions SET ConditionValue1=1282 WHERE ConditionTypeOrReference=40;';
 'applied-outside'='UPDATE conditions SET NegativeCondition=0 WHERE ConditionTypeOrReference=22 AND ElseGroup=0;';
 'applied-bank'='UPDATE conditions SET ConditionValue3=1 WHERE ConditionTypeOrReference=2;';
 'applied-quest'='UPDATE conditions SET ConditionValue1=39247 WHERE ConditionTypeOrReference=9;';
 'applied-stage'='UPDATE conditions SET ConditionValue2=5 WHERE SourceEntry=197092 AND ConditionTypeOrReference=44;';
 'applied-class'='UPDATE conditions SET ConditionValue1=1024 WHERE ConditionTypeOrReference=15;';
 'applied-missing-predicate'='DELETE FROM conditions WHERE SourceEntry=197092 AND ElseGroup=1 AND ConditionTypeOrReference=40;';
 'applied-same-count'="DELETE FROM conditions WHERE SourceEntry=197092 AND ElseGroup=1 AND ConditionTypeOrReference=40;INSERT INTO conditions VALUES(17,0,197092,0,1,4,0,1,0,0,0,0,'','custom');"
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state changed $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_11_frost_scene_equip.sql' AND hash='$hash' AND state='RELEASED';"
  if($live -notcontains '1'){throw 'Live Frost scene equip wave migration/hash differs'}
  $fullLive=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $fullLive "live-$table") -cne (Signature $rows "first-$table")){throw "Live differs from Frost scene equip regression $table"}}
 }
 'PASS full-schema TEMP Frost real scene callback:1 scoped script binding/1 complete mapping/33 exact cast predicates; native packages/flags/loot/alternate spells/quest objectives/outro unchanged, idempotence/25 full custom-partial refusals/live equality'
}
