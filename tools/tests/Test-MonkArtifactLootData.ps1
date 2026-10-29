param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/monk-loot-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_05_monk_real_loot.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_05_monk_real_loot.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql)
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];$sqlFile="$out/sql-$($script:queryIndex-1).sql";$stdoutFile="$out/sql-$($script:queryIndex-1).stdout"
   # Preserve complete native stdout/stderr bytes, including the tail after an
   # expected --force error; a PowerShell native pipeline lost part of a row.
   $process=Start-Process -FilePath C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -ArgumentList @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -RedirectStandardInput $sqlFile -RedirectStandardOutput $stdoutFile -RedirectStandardError $errorFile -WindowStyle Hidden -Wait -PassThru
   try{$process.Refresh();$code=$process.ExitCode}finally{$process.Dispose()}
   $rows=@(Get-Content $stdoutFile -Encoding UTF8);$errors=[string](Get-Content $errorFile -Raw -Encoding UTF8)
   if($Refusal){if($errors -notmatch 'Monk native loot/actions/admission differ; review before applying' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Monk loot refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Monk loot SQL failed: $errorFile"};return ,$rows
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/MonkLootOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{gameobject_template='entry IN(248086,251605)';gameobject_loot_template='Entry IN(248086,251605)';smart_scripts='entryorguid IN(248086,251605) AND source_type=1';conditions='(SourceTypeOrReferenceId=4 AND SourceGroup IN(248086,251605)) OR(SourceTypeOrReferenceId=22 AND SourceEntry IN(248086,251605) AND SourceId=1)'}
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
 $original=@((Signature $rows 'original-gameobject_template').Split("`n"));$repaired=@((Signature $rows 'first-gameobject_template').Split("`n"));if($original.Count -ne 2 -or $repaired.Count -ne 2){throw 'Expected two Monk chest templates'}
 for($i=0;$i -lt 2;++$i){$f=$original[$i].Split("`t");if($f[17] -ne '0'){throw 'Original native loot was not zero'};$f[17]=$f[0];if(($f -join "`t") -cne $repaired[$i]){throw 'Unexpected template field change'}}
 $conditions=@((Signature $rows 'first-conditions').Split("`n"));if($conditions.Count -ne 140){throw 'Expected140 exact predicates across both Monk pickups'}
 $rules=@($conditions|ForEach-Object {$f=$_.Split("`t");'rule'+"`t"+($f[0,1,2,4,5,6,7,8,9,10] -join "`t")})
 foreach($path in @(@(248086,128940,40570,1528,983,6),@(251605,128938,42765,1625,1137,7))){$go=$path[0];$item=$path[1];$quest=$path[2]
 foreach($scope in @(@(4,$go),@(22,1),@(22,3))){$entry=if($scope[0] -eq 4){$item}else{$go};$selected=@($conditions|Where-Object {$f=$_.Split("`t");[int]$f[0] -eq $scope[0] -and [int]$f[1] -eq $scope[1] -and [int]$f[2] -eq $entry});$groups=if($scope[0] -eq 4){2}else{4};if($selected.Count -ne 7*$groups){throw 'Unexpected Monk condition scope'}
  foreach($g in 0..($groups-1)){$v=@($selected|Where-Object {[int]$_.Split("`t")[4] -eq $g});$state=if(($g%2) -eq 0){9}else{28};$stage=$path[5]+[Math]::Floor($g/2)
   $expected=@(@(2,$item,1,$(if($scope[0] -eq 4){1}else{0}),$(if($scope[0] -eq 4){1}else{0})),@(15,512,0,0,0),@($state,$quest,0,0,0),@(22,$path[3],0,0,0),@(44,$path[4],$stage,0,0),@(8,$quest,0,0,1),@(36,0,0,0,0))|ForEach-Object {$_ -join ','}
   $actual=@($v|ForEach-Object {$a=$_.Split("`t");if($a[6] -ne '0' -or $a[11] -ne '0' -or $a[12] -ne ''){throw 'Unexpected condition target/error/script'};($a[5,7,8,9,10]) -join ','})
   if($v.Count -ne 7 -or @(Compare-Object $expected $actual -CaseSensitive).Count){throw "Unexpected native Monk predicate group $g"}
  }
 }}
 [IO.File]::WriteAllLines("$out/temp-rows.tsv",[string[]]$rules);[IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'template-flags'='UPDATE gameobject_template SET flags=flags+1 WHERE entry=251605;';
 'template-display'='UPDATE gameobject_template SET displayId=1 WHERE entry=251605;';
 'template-script'="UPDATE gameobject_template SET ScriptName='custom' WHERE entry=251605;";
 'template-link'='UPDATE gameobject_template SET Data1=1 WHERE entry=251605;';
 'loot-chance'='UPDATE gameobject_loot_template SET Chance=50 WHERE Entry=251605;';
 'loot-reference'='UPDATE gameobject_loot_template SET Reference=1 WHERE Entry=251605;';
 'loot-currency'='UPDATE gameobject_loot_template SET Currency=1 WHERE Entry=251605;';
 'loot-item'='UPDATE gameobject_loot_template SET Item=128306 WHERE Entry=251605;';
 'native-scene'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=251605 AND id=0;';
 'native-credit'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=251605 AND id=1;';
 'native-return'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=251605 AND id=3;';
 'return-link'='UPDATE smart_scripts SET link=0 WHERE entryorguid=251605 AND id=4;';
 'custom-condition'='UPDATE conditions SET ConditionValue3=1 WHERE SourceEntry=251605 AND SourceTypeOrReferenceId=22;';
 'partial-template'='UPDATE gameobject_template SET Data1=251605 WHERE entry=251605;';
 'partial-conditions'="INSERT INTO conditions VALUES(4,251605,127829,0,0,4,0,1,0,0,0,0,'','custom');";
 'applied-stage'='UPDATE conditions SET ConditionValue2=3 WHERE SourceTypeOrReferenceId=4 AND ConditionTypeOrReference=44;';
 'applied-quest'='UPDATE conditions SET ConditionValue1=41638 WHERE SourceTypeOrReferenceId=4 AND ConditionTypeOrReference=9;';
 'applied-bank'='UPDATE conditions SET ConditionValue3=0 WHERE SourceTypeOrReferenceId=4 AND ConditionTypeOrReference=2;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state changed $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_05_monk_real_loot.sql' AND hash='$hash' AND state='RELEASED';"
  if($live -notcontains '1'){throw 'Live Monk loot wave migration/hash differs'}
  $fullLive=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $fullLive "live-$table") -cne (Signature $rows "first-$table")){throw "Live differs from Monk loot regression $table"}}
 }
 'PASS full-schema TEMP Monk: only two Data1 fields plus140 exact predicates; unchanged10 native actions/loot/return links; idempotence,18 custom-partial refusals/live equality'
}
