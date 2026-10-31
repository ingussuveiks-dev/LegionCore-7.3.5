param([string]$MigrationFile,[switch]$VerifyLiveData,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-upper-voice-data"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_31_04_sharasdal_upper_intro_lines.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_31_04_sharasdal_upper_intro_lines.sql"}}
$migration=Get-Content $MigrationFile -Raw;$fixture=Get-Content "$PSScriptRoot/SharasdalUpperVoiceOriginal.json" -Raw|ConvertFrom-Json
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$script:queryIndex=0
function Query([string]$sql,[switch]$Refusal){
 $name="$out/sql-$script:queryIndex";++$script:queryIndex
 [IO.File]::WriteAllText("$name.sql",$sql+"`nSELECT '__SHARASDAL_VOICE_EOF__';`n")
 $previous=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$connection[3]
  $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile "$name.sql" -OutputFile "$name.stdout" -ErrorFile "$name.stderr"
  $rows=@(Get-Content "$name.stdout" -Encoding UTF8);$errors=[string](Get-Content "$name.stderr" -Raw -Encoding UTF8)
  if($Refusal){if($errors -notmatch 'Sharasdal upper intro context differs; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected voice refusal: $name.stderr"}}
  elseif($code -ne 0 -or $errors){throw "Voice SQL failed: $name.stderr"}
  if(!$rows.Count -or $rows[-1] -cne '__SHARASDAL_VOICE_EOF__'){throw "Incomplete native SQL output: $name.stdout"}
  return ,@($rows|Select-Object -SkipLast 1)
 }finally{$env:MYSQL_PWD=$previous}
}
$tables=@('conversation_data','conversation_creature','conversation_actor')
function Snapshot([string]$tag){$sql='';foreach($table in $tables){$sql+="SELECT '$tag-$table',t.* FROM $table t ORDER BY entry,id;"};$sql}
function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
$liveSql="SELECT 'line',entry,id,textId,unk1,unk2,idx FROM conversation_data WHERE entry=1868 ORDER BY entry,idx;SELECT 'creature',entry,id,creatureId,creatureGuid,unk1,unk2,duration FROM conversation_creature WHERE entry=1868 ORDER BY entry,id;SELECT 'actor-count',COUNT(*) FROM conversation_actor WHERE entry=1868;"
$before=Query $liveSql
$setup='';$baseline=''
foreach($table in $tables){
 $setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE entry=1868;CREATE TEMPORARY TABLE $table LIKE saved_$table;`n"
 $baseline+="DELETE FROM $table;`n"
 foreach($row in $fixture.$table){$cols=($row.PSObject.Properties.Name|ForEach-Object {"``$_``"}) -join ',';$values=($row.PSObject.Properties|ForEach-Object {[string]$_.Value}) -join ',';$baseline+="INSERT INTO $table($cols) VALUES($values);`n"}
}
$baseline+="INSERT INTO conversation_data(entry,id,idx,textId,unk1,unk2)VALUES(999999,1,0,99,7,7);INSERT INTO conversation_creature(entry,id,creatureId,creatureGuid,unk1,unk2,duration)VALUES(999999,0,102826,123,7,7,321);INSERT INTO conversation_actor(entry,id,actorId,creatureId,displayId,unk1,unk2,unk3,duration)VALUES(999999,0,33,111,222,0,0,0,999);"
$rows=Query ($setup+$baseline+(Snapshot 'original')+$migration+(Snapshot 'first')+$migration+(Snapshot 'second'))
foreach($table in $tables){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Upper voice migration not idempotent: $table"}}
if((Signature $rows 'original-conversation_actor') -cne (Signature $rows 'first-conversation_actor')){throw 'Actor overlay changed'}
$old=@((Signature $rows 'original-conversation_creature').Split("`n"));$new=@((Signature $rows 'first-conversation_creature').Split("`n"));if($old.Count -ne 3 -or $new.Count -ne 3){throw 'Missing two actors or unrelated sentinel'}
$edits=0
for($i=0;$i -lt $old.Count;++$i){$f=$old[$i].Split("`t");if($f.Count -ne 7){throw 'Creature schema changed'};if($f[0] -eq '1868' -and $f[1] -eq '1'){if($f[6] -ne '7800'){throw 'Original actor budget differs'};$f[6]='35016';++$edits};if(($f -join "`t") -cne $new[$i]){throw 'Unrelated creature payload changed'}}
if($edits -ne 1){throw 'Expected only second actor budget changed'}
$oldLines=@((Signature $rows 'original-conversation_data').Split("`n"));$newLines=@((Signature $rows 'first-conversation_data').Split("`n"));if($oldLines.Count -ne 3 -or $newLines.Count -ne 6){throw 'Expected exactly three missing native lines'}
$added=@("1868`t8701`t2`t14733`t0`t1","1868`t8702`t3`t28691`t0`t0","1868`t8703`t4`t30866`t0`t1")
if(Compare-Object @($oldLines+$added) $newLines){throw 'Original lines/offsets/unknowns/sentinel changed or new native payload differs'}
foreach($pair in @(@('original','baseline-conversation.tsv'),@('first','conversation.tsv'))){
 $lines=@(foreach($row in @((Signature $rows "$($pair[0])-conversation_data").Split("`n"))){$f=$row.Split("`t");if($f[0] -eq '1868'){"line`t$($f[0])`t$($f[1])`t$($f[3])`t$($f[4])`t$($f[5])`t$($f[2])"}})
 $lines+=@(foreach($row in @((Signature $rows "$($pair[0])-conversation_creature").Split("`n"))){$f=$row.Split("`t");if($f[0] -eq '1868'){"creature`t$row"}})
 if($lines.Count -ne $(if($pair[0] -eq 'original'){4}else{7})){throw 'Native upper fixture incomplete'}
 [IO.File]::WriteAllText("$out/$($pair[1])",($lines -join "`n")+"`n")
}
$mutations=@(
 'DELETE FROM conversation_data WHERE entry=1868 AND id=3961;',
 'UPDATE conversation_data SET textId=textId+1 WHERE entry=1868 AND id=3962;',
 'UPDATE conversation_data SET unk1=1 WHERE entry=1868 AND id=3961;',
 'UPDATE conversation_data SET unk2=3 WHERE entry=1868 AND id=3962;',
 'UPDATE conversation_data SET idx=2 WHERE entry=1868 AND id=3962;',
 'INSERT INTO conversation_data(entry,id,idx,textId,unk1,unk2)VALUES(1868,999998,5,99,0,0);',
 'DELETE FROM conversation_creature WHERE entry=1868 AND id=1;',
 'UPDATE conversation_creature SET creatureId=1 WHERE entry=1868 AND id=1;',
 'UPDATE conversation_creature SET creatureGuid=1 WHERE entry=1868 AND id=1;',
 'UPDATE conversation_creature SET unk1=1 WHERE entry=1868 AND id=0;',
 'UPDATE conversation_creature SET duration=7801 WHERE entry=1868 AND id=0;',
 'UPDATE conversation_creature SET duration=35016 WHERE entry=1868 AND id=1;',
 'INSERT INTO conversation_data(entry,id,idx,textId,unk1,unk2)VALUES(1868,8701,2,14733,0,1);',
 'INSERT INTO conversation_creature(entry,id,creatureId,creatureGuid,unk1,unk2,duration)VALUES(1868,2,105298,1,0,0,5000);',
 'INSERT INTO conversation_actor(entry,id,actorId,creatureId,displayId,unk1,unk2,unk3,duration)VALUES(1868,0,1,104853,0,0,0,0,1000);'
)

foreach($change in $mutations){$refused=Query ($setup+$baseline+$change+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal;foreach($table in $tables){if((Signature $refused "before-$table") -cne (Signature $refused "after-$table")){throw 'Refused custom/partial data was changed'}}}
$after=Query $liveSql
if(Compare-Object $before $after){throw 'Live World changed during temporary-only regression'}
if($VerifyLiveData){$actual=@($after|Where-Object {$_ -notlike "actor-count`t*"});if(Compare-Object $actual @(Get-Content "$out/conversation.tsv")){throw 'Live finale duration data not applied'};if($after[-1] -cne "actor-count`t0"){throw 'Live actor overlay changed'}}
'PASS native MariaDB TEMP-only upper1868: exactly3 matching native continuation lines/1 duration field, original2 line offsets/unknowns/2 actor bindings and unrelated sentinels retained, repeat idempotence,15 custom/partial/overlay refusals with full EOF/stderr. Live World unchanged by regression; applied readback only when selected. Upper caller/actual playback/full1066 remain separate.'
