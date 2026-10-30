param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/frost-loot-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_10_frost_real_loot.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_10_frost_real_loot.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql+"`nSELECT '__FROST_LOOT_SQL_COMPLETE__';`n")
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];$sqlFile="$out/sql-$($script:queryIndex-1).sql";$stdoutFile="$out/sql-$($script:queryIndex-1).stdout"
   # Preserve complete native stdout/stderr bytes, including the tail after an
   # expected --force error; a PowerShell native pipeline lost part of a row.
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile $sqlFile -OutputFile $stdoutFile -ErrorFile $errorFile
   $rows=@(Get-Content $stdoutFile -Encoding UTF8);$errors=[string](Get-Content $errorFile -Raw -Encoding UTF8)
   if($Refusal){if($errors -notmatch 'Frost native loot/action/admission differ; review before applying' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Frost loot refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Frost loot SQL failed: $errorFile"};if(!$rows.Count -or $rows[-1] -cne '__FROST_LOOT_SQL_COMPLETE__'){throw "Incomplete native SQL output: $stdoutFile"};return ,@($rows|Select-Object -SkipLast 1)
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/FrostLootOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{gameobject_template='entry=247386';gameobject_loot_template='Entry=247386';smart_scripts='entryorguid=247386 AND source_type=1';conditions='(SourceTypeOrReferenceId=4 AND SourceGroup=247386) OR(SourceTypeOrReferenceId=22 AND SourceEntry=247386 AND SourceId=1)'}
 $order=@{smart_scripts='entryorguid,source_type,id,link';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2';creature_text='CreatureID,GroupID,ID';creature_template='entry';gameobject_template='entry';gameobject_loot_template='Entry,Item,Reference,GroupId'}
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
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Not idempotent $table"};if($table -notin @('gameobject_template','conditions') -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Unrelated table changed $table"}}
 $original=@((Signature $rows 'original-gameobject_template').Split("`n"));$repaired=@((Signature $rows 'first-gameobject_template').Split("`n"));if($original.Count -ne 1 -or $repaired.Count -ne 1){throw 'Expected one Frost chest template'}
 for($i=0;$i -lt 1;++$i){$f=$original[$i].Split("`t");if($f[17] -ne '0'){throw 'Original native loot was not zero'};$f[17]=$f[0];if(($f -join "`t") -cne $repaired[$i]){throw 'Unexpected template field change'}}
 $conditions=@((Signature $rows 'first-conditions').Split("`n"));if($conditions.Count -ne 42){throw 'Expected42 exact predicates for actual Frost pickup'}
 $rules=@($conditions|ForEach-Object {$f=$_.Split("`t");'rule'+"`t"+($f[0,1,2,4,5,6,7,8,9,10] -join "`t")})
 foreach($path in @(,@(247386,128292,38990,1480,901,6))){$go=$path[0];$item=$path[1];$quest=$path[2]
 foreach($scope in @(@(4,$go),@(22,1))){$entry=if($scope[0] -eq 4){$item}else{$go};$selected=@($conditions|Where-Object {$f=$_.Split("`t");[int]$f[0] -eq $scope[0] -and [int]$f[1] -eq $scope[1] -and [int]$f[2] -eq $entry});$groups=if($scope[0] -eq 4){2}else{4};if($selected.Count -ne 7*$groups){throw 'Unexpected Frost condition scope'}
  foreach($g in 0..($groups-1)){$v=@($selected|Where-Object {[int]$_.Split("`t")[4] -eq $g});$state=if(($g%2) -eq 0){9}else{28};$stage=$path[5]+[Math]::Floor($g/2)
   $expected=@(@(2,$item,1,$(if($scope[0] -eq 4){1}else{0}),$(if($scope[0] -eq 4){1}else{0})),@(15,32,0,0,0),@($state,$quest,0,0,0),@(22,$path[3],0,0,0),@(44,$path[4],$stage,0,0),@(8,$quest,0,0,1),@(36,0,0,0,0))|ForEach-Object {$_ -join ','}
   $actual=@($v|ForEach-Object {$a=$_.Split("`t");if($a[6] -ne '0' -or $a[11] -ne '0' -or $a[12] -ne ''){throw 'Unexpected condition target/error/script'};($a[5,7,8,9,10]) -join ','})
   if($v.Count -ne 7 -or @(Compare-Object $expected $actual -CaseSensitive).Count){throw "Unexpected native Frost predicate group $g"}
  }
 }}
 [IO.File]::WriteAllLines("$out/temp-rows.tsv",[string[]]$rules);[IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'template-flags'='UPDATE gameobject_template SET flags=flags+1 WHERE entry=247386;';
 'template-display'='UPDATE gameobject_template SET displayId=1 WHERE entry=247386;';
 'template-script'="UPDATE gameobject_template SET ScriptName='custom' WHERE entry=247386;";
 'template-link'='UPDATE gameobject_template SET Data1=1 WHERE entry=247386;';
 'loot-chance'='UPDATE gameobject_loot_template SET Chance=50 WHERE Entry=247386;';
 'loot-reference'='UPDATE gameobject_loot_template SET Reference=1 WHERE Entry=247386;';
 'loot-currency'='UPDATE gameobject_loot_template SET Currency=1 WHERE Entry=247386;';
 'loot-item'='UPDATE gameobject_loot_template SET Item=128306 WHERE Entry=247386;';
 'native-scene'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=247386 AND id=0;';
 'native-target'='UPDATE smart_scripts SET target_type=1 WHERE entryorguid=247386 AND id=0;';
 'native-event'='UPDATE smart_scripts SET event_param1=1 WHERE entryorguid=247386 AND id=0;';
 'native-link'='UPDATE smart_scripts SET link=0 WHERE entryorguid=247386 AND id=0;';
 'custom-condition'="INSERT INTO conditions VALUES(22,1,247386,1,0,36,0,0,0,0,0,0,'','custom');";
 'secondary-spell'='UPDATE gameobject_template SET Data26=0 WHERE entry=247386;';
 'native-credit'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=247386 AND id=1;';
 'partial-template'='UPDATE gameobject_template SET Data1=247386 WHERE entry=247386;';
 'partial-conditions'="INSERT INTO conditions VALUES(4,247386,127829,0,0,4,0,1,0,0,0,0,'','custom');";
 'applied-stage'='UPDATE conditions SET ConditionValue2=3 WHERE SourceTypeOrReferenceId=4 AND ConditionTypeOrReference=44;';
 'applied-quest'='UPDATE conditions SET ConditionValue1=41638 WHERE SourceTypeOrReferenceId=4 AND ConditionTypeOrReference=9;';
 'applied-bank'='UPDATE conditions SET ConditionValue3=0 WHERE SourceTypeOrReferenceId=4 AND ConditionTypeOrReference=2;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state changed $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_10_frost_real_loot.sql' AND hash='$hash' AND state='RELEASED';"
  if($live -notcontains '1'){throw 'Live Frost loot wave migration/hash differs'}
  $fullLive=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $fullLive "live-$table") -cne (Signature $rows "first-$table")){throw "Live differs from Frost loot regression $table"}}
 }
 'PASS full-schema TEMP Frost: only Data1 plus42 exact predicates; unchanged native scene action/loot/native49193 LINK1; idempotence,20 custom-partial refusals/live equality'
}
