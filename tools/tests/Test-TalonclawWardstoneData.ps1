param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/talonclaw-wardstone-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_01_talonclaw_wardstone_admission.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_01_talonclaw_wardstone_admission.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql)
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];[Console]::OutputEncoding=[Text.UTF8Encoding]::new($false);$OutputEncoding=[Text.UTF8Encoding]::new($false);$ErrorActionPreference='Continue';$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --default-character-set=utf8mb4 --batch --raw --skip-column-names --force 2>$errorFile);$code=$LASTEXITCODE;$ErrorActionPreference='Stop';$errors=[string](Get-Content $errorFile -Raw)
   [IO.File]::WriteAllLines("$out/sql-$($script:queryIndex-1).stdout",[string[]]$rows)
   if($Refusal){if($errors -notmatch 'Talonclaw Wardstone/click/conditions differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Talonclaw Wardstone refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Talonclaw Wardstone SQL failed: $errorFile"};return ,$rows
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/TalonclawWardstoneOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='source_type=0 AND entryorguid IN(101325,100207,101360)';conditions='(SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(199728,199770,199823)) OR (SourceTypeOrReferenceId=18 AND SourceGroup=101325) OR (SourceTypeOrReferenceId=22 AND SourceEntry IN(101325,100207,101360))';creature_template='entry IN(101325,100207,101360)';npc_spellclick_spells='npc_entry=101325'}
 $order=@{smart_scripts='entryorguid,source_type,id,link';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2';creature_template='entry';npc_spellclick_spells='npc_entry,spell_id'}
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
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Not idempotent $table"};if($table -ne 'conditions' -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Unrelated table changed $table"}}
 $original=@((Signature $rows 'original-smart_scripts').Split("`n"));$repaired=@((Signature $rows 'first-smart_scripts').Split("`n"));$originalConditions=@((Signature $rows 'original-conditions').Split("`n"));$conditions=@((Signature $rows 'first-conditions').Split("`n"))
 if(($original -join "`n") -cne ($repaired -join "`n")){throw 'Any original SmartScript changed'}
 if($conditions.Count -ne $originalConditions.Count+44){throw 'Expected exactly44 additional predicates'}
 foreach($line in $originalConditions){if($conditions -cnotcontains $line){throw 'Original implicit target condition changed'}}
 $common=@(@(31,0,4,0,0),@(15,0,4,0,0),@(9,0,40385,0,0),@(22,0,1511,0,0),@(36,0,0,0,0),@(31,1,3,101325,0),@(22,1,1511,0,0),@(36,1,0,0,0))
 foreach($scope in @(@(18,101325,199728),@(17,0,199728),@(22,1,101325))){foreach($group in $(if($scope[0] -eq 18){@(0)}else{@(0,1)})){$expected=@($common|ForEach-Object {"$($scope[0]),$($scope[1]),$($scope[2]),0,$group,$($_ -join ','),0,0,"});if($scope[0] -ne 18){$expected+= "$($scope[0]),$($scope[1]),$($scope[2]),0,$group,44,1,973,$(3+$group),0,0,0,"};$actual=@($conditions|Where-Object {$f=$_.Split("`t");[int]$f[0] -eq $scope[0] -and [int]$f[1] -eq $scope[1] -and [int]$f[2] -eq $scope[2] -and [int]$f[4] -eq $group}|ForEach-Object {($_.Split("`t")[0..12]) -join ','});if($actual.Count -ne $expected.Count -or @(Compare-Object $actual $expected -CaseSensitive).Count){throw "Unexpected actual admission predicates $scope/group$group"}}}
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original);[IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$repaired)
 [IO.File]::WriteAllLines("$out/original-conditions.tsv",[string[]]$originalConditions);[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]$conditions)
 foreach($tag in @('original','first')){foreach($scope in @(@('click',18),@('cast',17),@('root',22))){$selected=@((Signature $rows "$tag-conditions").Split("`n")|Where-Object {$f=$_.Split("`t");[int]$f[0] -eq $scope[1]}|ForEach-Object {($_.Split("`t")[4..10]) -join "`t"});[IO.File]::WriteAllLines("$out/$tag-$($scope[0]).tsv",[string[]]$selected)}}
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'spellclick-caster'='UPDATE npc_spellclick_spells SET cast_flags=0 WHERE npc_entry=101325;';
 'spellclick-spell'='UPDATE npc_spellclick_spells SET spell_id=199770 WHERE npc_entry=101325;';
 'root-once'='UPDATE smart_scripts SET event_flags=0 WHERE entryorguid=101325 AND id=0;';
 'root-event'='UPDATE smart_scripts SET event_type=73 WHERE entryorguid=101325 AND id=0;';
 'root-criterion'='UPDATE smart_scripts SET action_param2=48069 WHERE entryorguid=101325 AND id=0;';
 'root-recipient'='UPDATE smart_scripts SET target_type=21 WHERE entryorguid=101325 AND id=0;';
 'native-lightning'='UPDATE smart_scripts SET action_param1=0 WHERE entryorguid=101325 AND id=1;';
 'native-transform'='UPDATE smart_scripts SET action_param1=0 WHERE entryorguid=101325 AND id=2;';
 'native-base'='UPDATE smart_scripts SET action_param1=0 WHERE entryorguid=101325 AND id=3;';
 'unknown-link4'='UPDATE smart_scripts SET link=0 WHERE entryorguid=101325 AND id=3;';
 'warder-trap'='UPDATE smart_scripts SET event_param1=1 WHERE entryorguid=100207 AND id=6;';
 'implicit-hit-entry'='UPDATE conditions SET ConditionValue2=100207 WHERE SourceEntry=199728;';
 'instant-kill-entry'='UPDATE conditions SET ConditionValue2=100207 WHERE SourceEntry=199823;';
 'instant-kill-mask'='UPDATE conditions SET SourceGroup=3 WHERE SourceEntry=199823;';
 'actor-click-flag'='UPDATE creature_template SET npcflag=0 WHERE entry=101325;';
 'custom-ai'="UPDATE creature_template SET ScriptName='custom' WHERE entry=101325;";
 'extra-row'="INSERT INTO smart_scripts(entryorguid,source_type,id,comment) VALUES(101325,0,99,'custom');";
 'custom-admission'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(17,199728,9,1);";
 'applied-stage'='UPDATE conditions SET ConditionValue2=2 WHERE SourceTypeOrReferenceId=22 AND ConditionTypeOrReference=44 AND ElseGroup=0;';
 'applied-caster-type'='UPDATE conditions SET ConditionValue1=3 WHERE SourceTypeOrReferenceId=17 AND ConditionTypeOrReference=31 AND ConditionTarget=0;';
 'applied-quest'='UPDATE conditions SET ConditionValue1=1 WHERE SourceTypeOrReferenceId=18 AND ConditionTypeOrReference=9;';
 'partial'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(17,199728,22,1511);"
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state changed $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_01_talonclaw_wardstone_admission.sql' AND hash='$hash' AND state='RELEASED';"
  if($live -notcontains '1'){throw 'Live Talonclaw Wardstone wave migration/hash differs'}
  $fullLive=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $fullLive "live-$table") -cne (Signature $rows "first-$table")){throw "Live differs from Talonclaw Wardstone regression $table"}}
 }
 'PASS full-schema TEMP Wardstone admission:44 exact predicates (8 visible click,18 actual cast,18 independent hit); all scripts/templates/click metadata/native source13 target and death masks unchanged; repeat/live readback and22 custom/partial refusals'
}
