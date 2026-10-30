param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/fire-iceborn-story-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_25_fire_iceborn_story.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_25_fire_iceborn_story.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql+"`nSELECT '__ICEBORN_STORY_SQL_COMPLETE__';`n")
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];$sqlFile="$out/sql-$($script:queryIndex-1).sql";$stdoutFile="$out/sql-$($script:queryIndex-1).stdout"
   # Preserve complete native stdout/stderr bytes, including the tail after an
   # expected --force error; a PowerShell native pipeline lost part of a row.
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile $sqlFile -OutputFile $stdoutFile -ErrorFile $errorFile
   $rows=@(Get-Content $stdoutFile -Encoding UTF8);$errors=[string](Get-Content $errorFile -Raw -Encoding UTF8)
   if($Refusal){if($errors -notmatch 'Fire Iceborn story/context differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Fire Iceborn story refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Fire Iceborn story SQL failed: $errorFile"}
   if(!$rows.Count -or $rows[-1] -cne '__ICEBORN_STORY_SQL_COMPLETE__'){throw "Incomplete native SQL output: $stdoutFile"}
   return ,@($rows|Select-Object -SkipLast 1)
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/FireIcebornStoryOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='source_type IN(0,9) AND entryorguid IN(98760,99605,99600,99615)';conditions='(SourceTypeOrReferenceId=22 AND SourceEntry IN(98760,99605,99600,99615)) OR(SourceTypeOrReferenceId IN(13,17) AND SourceEntry=197760)';creature_text='CreatureID IN(98760,99605,99600,99615)';creature_template='entry IN(98760,99605,99600,99615)';creature_template_wdb='Entry IN(98760,99605,99600,99615)';creature='id IN(98760,99605,99600,99615)';conversation_data='entry=786';conversation_creature='entry=786';conversation_actor='entry=786';spell_script_names='spell_id=197760'}
 $order=@{smart_scripts='entryorguid,source_type,id,link';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2';creature_text='CreatureID,GroupID,ID';creature_template='entry';creature_template_wdb='Entry';creature='guid';conversation_data='entry,id,idx';conversation_creature='entry,id';conversation_actor='entry,id';spell_script_names='spell_id,ScriptName'}
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
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Not idempotent $table"};if($table -notin @('smart_scripts','conversation_actor') -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Native return data changed $table"}}
 $original=@((Signature $rows 'original-smart_scripts').Split("`n"));$applied=@((Signature $rows 'first-smart_scripts').Split("`n"))
 if($original.Count -ne @($fixture.smart_scripts).Count -or $applied.Count -ne $original.Count){throw 'Missing native Fire Iceborn story/context rows'}
 $edits=0;for($i=0;$i -lt $original.Count;++$i){$f=$original[$i].Split("`t")
  if($f[0] -eq '98760' -and $f[1] -eq '0' -and $f[2] -eq '7'){if($f[14] -ne '85' -or $f[15] -ne '197760' -or $f[16] -ne '0' -or $f[21] -ne '7' -or $f[22] -ne '0'){throw 'Original Iceborn caster tuple differs'};$f[16]='128';$f[21]='130';$f[22]='70';$edits+=3}
  if(($f -join "`t") -cne $applied[$i]){throw 'Unexpected Iceborn row mutation'}
 };if($edits -ne 3){throw 'Expected exactly3 native Iceborn caster fields'}
 $actor=(Signature $rows 'original-conversation_actor').Split("`t");if($actor.Count -ne 9 -or $actor[0] -ne '786' -or $actor[1] -ne '0' -or $actor[2] -ne '50151' -or $actor[3] -ne '99615' -or $actor[8] -ne '6000'){throw 'Original Lyandra actor definition differs'};$actor[8]='48827';if(($actor -join "`t") -cne (Signature $rows 'first-conversation_actor')){throw 'Unexpected Lyandra actor field change'}
 foreach($pair in @(@('original','baseline-row'),@('first','row'))){$native=@((Signature $rows "$($pair[0])-smart_scripts").Split("`n")|Where-Object {$f=$_.Split("`t");$f[0] -eq '98760' -and $f[1] -eq '0' -and $f[2] -eq '7'});if($native.Count -ne 1){throw 'Missing native Iceborn row'};$f=$native[0].Split("`t");[IO.File]::WriteAllText("$out/$($pair[1]).tsv",(($f[14],$f[15],$f[16],$f[21],$f[22]) -join "`t")+"`n")}
 foreach($pair in @(@('original','baseline-conversation'),@('first','conversation'))){$payload=@();foreach($name in @('actor','creature','line')){$table=@{actor='conversation_actor';creature='conversation_creature';line='conversation_data'}[$name];$payload+=@((Signature $rows "$($pair[0])-$table").Split("`n")|Where-Object {$_}|ForEach-Object {"$name`t$_"})};[IO.File]::WriteAllLines("$out/$($pair[1]).tsv",[string[]]$payload)}
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original)
 [IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$applied)
 [IO.File]::WriteAllLines("$out/texts.tsv",[string[]]((Signature $rows 'first-creature_text').Split("`n")))
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'native-story'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=98760 AND id=7;';
 'native-flags'='UPDATE smart_scripts SET action_param2=2 WHERE entryorguid=98760 AND id=7;';
 'native-credit'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=98760 AND id=4;';
 'native-death-link'='UPDATE smart_scripts SET link=0 WHERE entryorguid=98760 AND id=4;';
 'native-voice-owner'='UPDATE smart_scripts SET target_type=1 WHERE entryorguid=98760 AND id=6;';
 'native-block-signal'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=98760 AND id=3;';
 'native-block-proximity'='UPDATE smart_scripts SET event_param2=1 WHERE entryorguid=99605 AND id=0;';
 'native-geist-signal'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=99600 AND id=1;';
 'unknown-lyandra-link'='UPDATE smart_scripts SET link=0 WHERE entryorguid=99615 AND id=21;';
 'native-ai'="UPDATE creature_template SET ScriptName='custom' WHERE entry=98760;";
 'native-spawn'='UPDATE creature SET phaseMask=2 WHERE id=98760;';
 'native-actor'='UPDATE conversation_actor SET displayId=1 WHERE entry=786;';
 'native-line-offset'='UPDATE conversation_data SET textId=1 WHERE entry=786 AND id=1873;';
 'native-line-chain'='UPDATE conversation_data SET id=1 WHERE entry=786 AND id=1872;';
 'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(17,197760,9,1);";
 'custom-spell-script'="INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(197760,'custom');";
 'partial-owner-only'='UPDATE smart_scripts SET action_param2=128,target_type=130,target_param1=70 WHERE entryorguid=98760 AND id=7;';
 'partial-duration-only'='UPDATE conversation_actor SET duration=48827 WHERE entry=786;';
 'partial-target'='UPDATE smart_scripts SET target_type=130 WHERE entryorguid=98760 AND id=7;';
 'applied-range'='UPDATE smart_scripts SET target_param1=71 WHERE entryorguid=98760 AND id=7;';
 'applied-duration'='UPDATE conversation_actor SET duration=48826 WHERE entry=786;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom Fire Iceborn story data mutated $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_25_fire_iceborn_story.sql' AND hash='$hash' AND state='RELEASED';";if($live -notcontains '1'){throw 'Live Fire Iceborn story migration/hash differs'}
  $live=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $live "live-$table") -cne (Signature $rows "first-$table")){throw "Live native Fire Iceborn story context differs $table"}}
 }
 'PASS full10table Fire Iceborn story: exactly4 fields; native197760/786 Player owner plus48827+5000 lifetime; four original lines/Fire credit/block-geist/final-boss unknownLINK context unchanged; twice-idempotent/21 partial-custom refusals/live when requested'
}
