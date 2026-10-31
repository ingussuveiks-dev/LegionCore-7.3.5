param([string]$MigrationFile='',[switch]$VerifyLiveData,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-acquisition-data"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_31_03_sharasdal_acquisition.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_31_03_sharasdal_acquisition.sql"}}
$migration=Get-Content $MigrationFile -Raw;$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$script:index=0
function Query([string]$sql,[switch]$Refusal){
 $prefix="$out/sql-$script:index";++$script:index
 [IO.File]::WriteAllText("$prefix.sql",$sql+"`nSELECT '__SHARASDAL_ACQUISITION_EOF__';`n")
 $previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile "$prefix.sql" -OutputFile "$prefix.stdout" -ErrorFile "$prefix.stderr"
  $rows=@(Get-Content "$prefix.stdout" -Encoding UTF8);$errors=[string](Get-Content "$prefix.stderr" -Raw -Encoding UTF8)
  if($Refusal){if($errors -notmatch 'Sharasdal acquisition context differs; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected refusal: $prefix.stderr"}}
  elseif($code -ne 0 -or $errors){throw "Acquisition SQL failed: $prefix.stderr"}
  if(!$rows.Count -or $rows[-1] -cne '__SHARASDAL_ACQUISITION_EOF__'){throw "Incomplete native SQL: $prefix.stdout"}
  return ,@($rows|Select-Object -SkipLast 1)
 }finally{$env:MYSQL_PWD=$previous}
}
$tables=[ordered]@{
 gameobject_template='entry IN(249342,254087)';gameobject='id=249342';gameobject_loot_template='entry IN(249342,254087)';
 creature='id=105819';creature_template='entry=105819';npc_spellclick_spells='npc_entry IN(105819,98775)';
 spell_target_position='id IN(209683,210252,230156)';spell_scene='MiscValue IN(1255,1218)';spell_script_names='spell_id IN(209683,209684,209685,208171)';
 conditions='(SourceTypeOrReferenceId=18 AND SourceGroup IN(105819,98775)) OR (SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(209683,209684,209685)) OR (SourceTypeOrReferenceId=23 AND SourceGroup=8106)';quest_objectives='QuestID=40341'
}
$setup='';$snapshot='';$live=''
foreach($table in $tables.Keys){$setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($tables[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n";$order=switch($table){'gameobject_template'{'entry'}'gameobject'{'guid'}'gameobject_loot_template'{'entry,item,currency'}'creature'{'guid'}'creature_template'{'entry'}'npc_spellclick_spells'{'npc_entry,spell_id'}'spell_target_position'{'id'}'spell_scene'{'MiscValue,SceneScriptPackageID'}'spell_script_names'{'spell_id,ScriptName'}'conditions'{'SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3'}'quest_objectives'{'ID'}};$snapshot+="SELECT 'TAG-$table',t.* FROM $table t ORDER BY $order;";$live+="SELECT 'live-$table',t.* FROM $table t WHERE $($tables[$table]) ORDER BY $order;"}
function Snap([string]$tag){$snapshot.Replace('TAG',$tag)}
function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
$before=Query $live
$chest=Query 'SELECT type,data1,data2,data3,data4,data5,data25,data30 FROM gameobject_template WHERE entry=249342;'
if($chest.Count -ne 1 -or $chest[0] -notmatch '^3\t(?:0|249342)\t1\t0\t0\t0\t0\t0$'){throw 'Actual native chest is not the verified ordinary/non-vein/non-personal release path'}
[IO.File]::WriteAllLines("$out/native-chest.tsv",[string[]]$chest)
# First validate current state with the strict migration before reconstructing
# a known original TEMP graph. This cannot normalize away live custom data.
$original="UPDATE gameobject_template SET data1=0 WHERE entry=249342;DELETE FROM spell_target_position WHERE id=209683;DELETE FROM spell_script_names WHERE spell_id IN(209683,209684,209685);DELETE FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=105819;"
$baseline=$setup+$migration+$original
$rows=Query ($baseline+(Snap 'original')+$migration+(Snap 'first')+$migration+(Snap 'second')+"SELECT 'binding',spell_id,ScriptName FROM spell_script_names WHERE spell_id IN(209683,209684,209685) ORDER BY spell_id,ScriptName;SELECT 'return-row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=105819 ORDER BY ElseGroup,ConditionTypeOrReference;UPDATE conditions SET ConditionValue3=0 WHERE SourceTypeOrReferenceId=18 AND SourceGroup=105819 AND ConditionTypeOrReference=2;"+$migration+(Snap 'bank-upgraded')+$original+(Snap 'restored'))
[IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
foreach($table in $tables.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Not idempotent: $table"};if((Signature $rows "first-$table") -cne (Signature $rows "bank-upgraded-$table")){throw "Inventory-only pre-commit upgrade differs: $table"};if((Signature $rows "original-$table") -cne (Signature $rows "restored-$table")){throw "Changed fields/rows outside data1/new209683/four loaders/eighteen return rows: $table"};if($table -notin @('gameobject_template','spell_target_position','spell_script_names','conditions') -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Unrelated table changed: $table"}}
$data=@($rows|Where-Object {$_ -like "return-row`t*"}|ForEach-Object {$_.Substring(11)});if($data.Count -ne 18){throw 'Missing complete/incomplete return ElseGroups'};[IO.File]::WriteAllLines("$out/return-rows.tsv",[string[]]$data)
$expected=@("209683`tspell_sharasdal_acquisition","209684`taura_sharasdal_acquisition","209684`tspell_sharasdal_acquisition","209685`tspell_sharasdal_acquisition")
if(Compare-Object $expected @($rows|Where-Object {$_ -like "binding`t*"}|ForEach-Object {$_.Substring(8)})){throw 'Missing exact matching acquisition loaders'}
$mutations=@(
 'UPDATE gameobject_template SET data1=17 WHERE entry=249342;',
 'UPDATE gameobject_template SET ScriptName=''custom'' WHERE entry=249342;',
 'UPDATE gameobject_loot_template SET Reference=1 WHERE entry=249342;',
 'UPDATE gameobject_loot_template SET Chance=99 WHERE entry=249342;',
 'INSERT INTO gameobject_loot_template(Entry,Item) VALUES(249342,128934);',
 'UPDATE gameobject SET PhaseId=''6120'' WHERE id=249342;',
 'UPDATE creature SET PhaseId=''7002'' WHERE id=105819;',
 'UPDATE npc_spellclick_spells SET cast_flags=3 WHERE npc_entry=105819;',
 'INSERT INTO npc_spellclick_spells(npc_entry,spell_id,cast_flags,user_type,add_npc_flag) VALUES(105819,209683,1,0,1);',
 'UPDATE spell_target_position SET target_map=1285 WHERE id=210252;',
 'UPDATE spell_scene SET ScriptName=''custom'' WHERE MiscValue=1255;',
 'UPDATE quest_objectives SET StorageIndex=9 WHERE ID=283384;',
 'INSERT INTO spell_script_names VALUES(209684,''custom'');',
 'INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(18,105819,210252,22,1);',
 'INSERT INTO spell_target_position VALUES(209683,1600,1,2,3,0);',
 'INSERT INTO spell_target_position VALUES(209683,1600,50.5781,802.392,805.731,0);',
 'INSERT INTO conditions(SourceTypeOrReferenceId,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(17,209685,22,1);',
 'INSERT INTO spell_script_names VALUES(209683,''spell_sharasdal_acquisition'');',
 ($migration+'UPDATE conditions SET ConditionValue2=1 WHERE SourceTypeOrReferenceId=18 AND SourceGroup=105819 AND ConditionTypeOrReference=44;'),
 ($migration+'UPDATE conditions SET ConditionValue3=0 WHERE SourceTypeOrReferenceId=18 AND SourceGroup=105819 AND ConditionTypeOrReference=2 AND ElseGroup=0;')
)
for($i=0;$i -lt $mutations.Count;++$i){$probe=Query ($baseline+$mutations[$i]+(Snap 'before')+$migration+(Snap 'after')) -Refusal;foreach($table in $tables.Keys){if((Signature $probe "before-$table") -cne (Signature $probe "after-$table")){throw "Custom mutation$i modified $table"}}}
$after=Query $live;if(Compare-Object $before $after){throw 'Read-only TEMP regression changed live World data'}
if($VerifyLiveData){$hash=(Get-FileHash $MigrationFile -Algorithm SHA1).Hash;$released=Query "SELECT hash,state FROM updates WHERE name='2026_10_31_03_sharasdal_acquisition.sql';";if($released -notcontains "$hash`tRELEASED"){throw 'Canonical updater hash/state mismatch'};foreach($table in $tables.Keys){if((Signature $rows "first-$table") -cne (Signature $after "live-$table")){throw "Live differs from tested acquisition: $table"}}}
'PASS eleven TEMP tables, strict original/applied and exact pre-commit bank upgrade, repeated migration, twenty custom/partial refusals with full EOF and zero live mutation; real native loot link/destination/four loaders/eighteen return conditions, phase/return target/native scene/unrelated rows retained'
