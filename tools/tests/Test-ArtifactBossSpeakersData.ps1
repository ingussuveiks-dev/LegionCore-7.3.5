param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-boss-speakers-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_29_24_artifact_boss_speakers.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_29_24_artifact_boss_speakers.sql"}}
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
   if($Refusal){if($errors -notmatch 'Artifact boss speech/sequence/loot differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Artifact boss speaker refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Artifact boss speaker SQL failed: $errorFile"};return ,$rows
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/ArtifactBossSpeakersOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='(source_type=0 AND entryorguid IN(101148,108168,102247,108202)) OR (source_type=1 AND entryorguid IN(247690,251049)) OR (source_type=9 AND entryorguid IN(101148,108168))';conditions='SourceTypeOrReferenceId=22 AND SourceEntry IN(101148,108168,102247,108202,247690,251049)';creature_text='CreatureID IN(101148,108168,102247,108202)';creature_template='entry IN(101148,108168,102247,108202)';gameobject_template='entry IN(247690,251049)';gameobject_loot_template='Entry IN(247690,251049)'}
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
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Not idempotent $table"};if($table -ne 'smart_scripts' -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Unrelated table changed $table"}}
 $original=@((Signature $rows 'original-smart_scripts').Split("`n"));$repaired=@((Signature $rows 'first-smart_scripts').Split("`n"));$conditions=@((Signature $rows 'first-conditions').Split("`n"))
 if($original.Count -ne @($fixture.smart_scripts).Count -or $repaired.Count -ne $original.Count){throw 'Original/repaired row count differs'}
 $edits=0
 for($i=0;$i -lt $original.Count;++$i){$f=$original[$i].Split("`t");if($f[1] -eq '0' -and (($f[0] -eq '101148' -and [int]$f[2] -in @(1,21,22,23)) -or ($f[0] -eq '108168' -and [int]$f[2] -in @(0,6)))){if($f[21] -ne $(if($f[0] -eq '101148' -and $f[2] -eq '1'){'7'}else{'2'})){throw 'Original speaker differs'};$f[21]='1';++$edits};if(($f -join "`t") -cne $repaired[$i]){throw "Unexpected row mutation $i"}}
 if($edits -ne 6){throw 'Expected exactly six speaker fields'}
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original);[IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$repaired);[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]$conditions)
 [IO.File]::WriteAllLines("$out/texts.tsv",[string[]]@((Signature $rows 'first-creature_text').Split("`n")))
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'lost-text-over'='UPDATE smart_scripts SET event_type=61 WHERE entryorguid=101148 AND id=22;';
 'farthing-first-text'='UPDATE smart_scripts SET action_param1=0 WHERE entryorguid=101148 AND id=21;';
 'farthing-delay'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=101148 AND id=22;';
 'combat-target'='UPDATE smart_scripts SET target_type=1 WHERE entryorguid=101148 AND id=3;';
 'soul-cast'='UPDATE smart_scripts SET action_param1=0 WHERE entryorguid=102247 AND id=0;';
 'soul-transition'='UPDATE smart_scripts SET action_param1=0 WHERE entryorguid=101148 AND id=29;';
 'real-weapon-ack'='UPDATE smart_scripts SET link=0 WHERE entryorguid=101148 AND id=30;';
 'balaadur-text'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=108168 AND id=6;';
 'balaadur-health'='UPDATE smart_scripts SET event_param2=1 WHERE entryorguid=108168 AND id=0;';
 'unknown-balaadur-link'='UPDATE smart_scripts SET link=0 WHERE entryorguid=108168 AND id=7;';
 'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,22,101148,9,1);";
 'custom-ai'="UPDATE creature_template SET ScriptName='custom' WHERE entry=101148;";
 'native-text'='UPDATE creature_text SET BroadcastTextID=1 WHERE CreatureID=108168 AND GroupID=0;';
 'weapon-link'='UPDATE gameobject_template SET Data1=0 WHERE entry=247690;';
 'weapon-loot'='UPDATE gameobject_loot_template SET Chance=1 WHERE Entry=247690;';
 'missing-row'='DELETE FROM smart_scripts WHERE entryorguid=108168 AND id=0;';
 'applied-speaker'='UPDATE smart_scripts SET target_type=7 WHERE entryorguid=101148 AND id=22;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state changed $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_29_24_artifact_boss_speakers.sql' AND hash='$hash' AND state='RELEASED';"
  if($live -notcontains '1'){throw 'Live Artifact boss speaker wave migration/hash differs'}
  $fullLive=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $fullLive "live-$table") -cne (Signature $rows "first-$table")){throw "Live differs from Artifact boss speaker regression $table"}}
 }
 'PASS full-schema TEMP Farthing/Balaadur speaker replay/idempotence: only six TALK target fields; original delays/texts/casts/summons/criteria/finale/weapon/native loot and unknownBalaadurLINK8 retained; seventeen custom-state refusals'
}
