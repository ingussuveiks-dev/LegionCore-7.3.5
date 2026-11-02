param([string]$MigrationFile,[switch]$VerifyLiveData,[string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/monk-student-replies/$(Get-Date -Format yyyyMMdd-HHmmss-fff)")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out=[IO.Path]::GetFullPath($OutputDirectory)
if(!$MigrationFile){$MigrationFile="$repo/sql/updates/world/2026_11_02_03_monk_student_replies.sql"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo) -Test {
 $context=Get-Content "$PSScriptRoot/MonkStudentRepliesOriginal.json" -Raw|ConvertFrom-Json
 $migration=[IO.File]::ReadAllText($MigrationFile)
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$script:index=0
 function Query([string]$sql,[switch]$Refusal){
  $stem="$out/sql-$script:index";++$script:index;[IO.File]::WriteAllText("$stem.sql",$sql+"`nSELECT 'EOF','monk-student-data';`n")
  $prior=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$connection[3];$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile "$stem.sql" -OutputFile "$stem.stdout" -ErrorFile "$stem.stderr"}finally{$env:MYSQL_PWD=$prior}
  $rows=@(Get-Content "$stem.stdout");$errors=@(Get-Content "$stem.stderr"|Where-Object {$_ -match '\S'})
  if($rows[-1] -cne "EOF`tmonk-student-data"){throw 'Native SQL did not reach full EOF'}
  if($Refusal){if($errors.Count -ne 4 -or $errors[0] -cne '--------------' -or $errors[1] -cne 'INSERT INTO monk_student_guard VALUES(@monk_student_safe)' -or $errors[2] -cne '--------------' -or $errors[3] -notmatch '^ERROR 4025 .*monk_student_native_guard'){throw 'Refusal contains unexplained native SQL diagnostics'}}
  elseif($code -or $errors.Count){throw 'Native SQL exit/stderr failure; evidence retained'}
  $rows
 }
 function Literal($value){if($null -eq $value){return 'NULL'};if($value -is [string]){if(!$value.Length){return "''"};return 'CONVERT(0x'+[Convert]::ToHexString([Text.Encoding]::UTF8.GetBytes($value))+' USING utf8mb4)'};[Convert]::ToString($value,[Globalization.CultureInfo]::InvariantCulture)}
 function InsertRow([string]$table,$row){'INSERT INTO '+$table+'('+[string]::Join(',',@($row.PSObject.Properties.Name|ForEach-Object {'`'+$_+'`'}))+') VALUES('+[string]::Join(',',@($row.PSObject.Properties.Value|ForEach-Object {Literal $_}))+');' + "`n"}
 $tables=@($context.filters.PSObject.Properties.Name);$setup='';$expected='DELETE FROM creature_text WHERE CreatureID=98074;' + "`n"
 foreach($table in $tables){
  $setup+="CREATE TEMPORARY TABLE schema_$table LIKE $table;CREATE TEMPORARY TABLE $table LIKE schema_$table;DROP TEMPORARY TABLE schema_$table;`n"
  foreach($row in @($context.tables.$table)){$setup+=InsertRow $table $row}
 }
 $setup+="INSERT INTO creature_text(CreatureID,GroupID,ID,Text,Type,Language,Probability,Emote,Duration,Sound,BroadcastTextID,MinTimer,MaxTimer,SpellID,comment) VALUES(1,0,0,'Unrelated native sentinel',12,0,100,0,0,0,0,0,0,0,'Must remain unchanged');`n"
 foreach($row in $context.expected_text){$expected+=InsertRow creature_text $row}
 function Snapshot([string]$tag,[switch]$Scoped){$result='';foreach($table in $tables){$where=if($Scoped){' WHERE '+$context.filters.$table}else{''};$result+="SELECT '$tag-$table',t.* FROM $table t$where ORDER BY $($context.orders.$table);`n"};$result}
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
 $expectedRows=Query ($setup+$expected+(Snapshot expected)+(Snapshot scoped -Scoped))
 $rows=Query ($setup+(Snapshot before)+$migration+(Snapshot first)+$migration+(Snapshot second))
 foreach($table in $tables){
  if((Signature $rows "first-$table") -cne (Signature $rows "second-$table") -or (Signature $rows "first-$table") -cne (Signature $expectedRows "expected-$table")){throw "Full-column delta/idempotence differs: $table"}
  if($table -ne 'creature_text' -and (Signature $rows "before-$table") -cne (Signature $rows "first-$table")){throw "Other native context changed: $table"}
 }
 # Both selected native responses remain unvoiced. Keep timed3 clocks and its
 # duplicate per-text ID0, but distinct0/1 IDs in the scripted default group.
 $defaults=@($context.expected_text|Where-Object GroupID -eq 0)
 if($defaults.Count -ne 2 -or ([string]::Join(',',@($defaults.ID|Sort-Object))) -cne '0,1' -or ([string]::Join(',',@($defaults.BroadcastTextID|Sort-Object))) -cne '99968,99970'){throw 'Scripted default identities differ'}
 foreach($row in $context.expected_text){if($row.Sound -ne 0 -or $row.Duration -ne 0 -or ($row.GroupID -eq 0 -and ($row.MinTimer -ne 0 -or $row.MaxTimer -ne 0))){throw 'Student voice/duration/timer invariant differs'}}
 $partialDefault=InsertRow creature_text $defaults[0]
 $allDefaults=[string]::Join('',@($defaults|ForEach-Object {InsertRow creature_text $_}))
 $variants=@{
  'partial-one-default'=$partialDefault
  'partial-corrected-sound-only'='UPDATE creature_text SET Sound=0 WHERE BroadcastTextID=99968;'
  'partial-both-default-old-sound'=$allDefaults
  'custom-group-zero'="INSERT INTO creature_text(CreatureID,GroupID,ID,Text) VALUES(98074,0,0,'Custom reply');"
  'extra-timed-row'="INSERT INTO creature_text(CreatureID,GroupID,ID,Text) VALUES(98074,3,1,'Custom timer');"
  'other-group'="INSERT INTO creature_text(CreatureID,GroupID,ID,Text) VALUES(98074,9,0,'Custom group');"
  'changed-native-broadcast'='UPDATE creature_text SET BroadcastTextID=99969 WHERE BroadcastTextID=99968;'
  'different-sound'='UPDATE creature_text SET Sound=1 WHERE BroadcastTextID=99968;'
  'changed-silent-reply'="UPDATE creature_text SET Text='Custom text' WHERE BroadcastTextID=99970;"
  'missing-reply'='DELETE FROM creature_text WHERE BroadcastTextID=99970;'
  'changed-timer-min'='UPDATE creature_text SET MinTimer=1 WHERE BroadcastTextID=99968;'
  'changed-timer-max'='UPDATE creature_text SET MaxTimer=1 WHERE BroadcastTextID=99968;'
  'changed-timed-group'='UPDATE creature_text SET GroupID=0 WHERE CreatureID=98074;'
  'changed-emote'='UPDATE creature_text SET Emote=0 WHERE BroadcastTextID=99968;'
  'changed-duration'='UPDATE creature_text SET Duration=1 WHERE BroadcastTextID=99968;'
  'changed-type'='UPDATE creature_text SET Type=14 WHERE BroadcastTextID=99968;'
  'changed-language'='UPDATE creature_text SET Language=1 WHERE BroadcastTextID=99968;'
  'changed-probability'='UPDATE creature_text SET Probability=0 WHERE BroadcastTextID=99968;'
  'changed-timed-id'='UPDATE creature_text SET ID=1 WHERE BroadcastTextID=99968;'
  'changed-spell'='UPDATE creature_text SET SpellID=1 WHERE BroadcastTextID=99968;'
  'custom-template-script'="UPDATE creature_template SET ScriptName='custom' WHERE entry=98074;"
  'custom-template-ai'="UPDATE creature_template SET AIName='SmartAI' WHERE entry=98074;"
  'changed-model'='UPDATE creature_template_wdb SET DisplayId1=1 WHERE entry=98074;'
  'changed-source-map'='UPDATE creature SET map=1 WHERE guid=362259;'
  'changed-source-phase'="UPDATE creature SET PhaseId='7001' WHERE guid=362259;"
  'missing-student'='DELETE FROM creature WHERE guid=362259;'
  'custom-smart'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,target_type,comment) VALUES(98074,0,0,0,1,1,'Custom');"
 }
 foreach($name in $variants.Keys){
  $refusal=Query ($setup+$variants[$name]+(Snapshot before)+$migration+(Snapshot after)) -Refusal
  foreach($table in $tables){if((Signature $refusal "before-$table") -cne (Signature $refusal "after-$table")){throw "Refused $name mutated $table"}}
 }
 if($VerifyLiveData){$live=Query ('SET SESSION TRANSACTION READ ONLY;'+(Snapshot live -Scoped));foreach($table in $tables){if((Signature $live "live-$table") -cne (Signature $expectedRows "scoped-$table")){throw "Live student context differs: $table"}}}
 "PASS native SQL:8 complete TEMP schemas/full snapshots/unrelated sentinel, exactly2 default reply rows and1 wrong sound removal, timed3 text/deadlines retained, idempotence,$($variants.Count) unchanged refusals; live=$VerifyLiveData"
}
