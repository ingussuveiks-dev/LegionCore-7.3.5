param([string]$MigrationFile='',[switch]$VerifyLiveData,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-ascent-admission-data"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_31_05_sharasdal_ascent_admission.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_31_05_sharasdal_ascent_admission.sql"}}
$migration=Get-Content $MigrationFile -Raw
$scope='(SourceTypeOrReferenceId=17 AND SourceEntry IN(208580,209992)) OR (SourceTypeOrReferenceId=18 AND SourceGroup=38821 AND SourceEntry=208580)'
$tables=[ordered]@{
 conditions=@("($scope) OR (SourceTypeOrReferenceId=13 AND SourceEntry IN(208580,209992))",'SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition');
 npc_spellclick_spells=@('npc_entry=38821','npc_entry,spell_id');
 creature=@('id=38821','guid');spell_scene=@('MiscValue=1247','MiscValue');spell_target_position=@('id=209992','id');
 spell_linked_spell=@('ABS(spell_trigger) IN(208580,209992) OR ABS(spell_effect) IN(208580,209992)','spell_trigger,spell_effect,type');
 spell_scene_event=@('MiscValue=1247','MiscValue,Event,trigerSpell,MonsterCredit');spell_script_names=@('spell_id IN(208580,209992)','spell_id,ScriptName')}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$script:index=0
function Query([string]$sql,[switch]$Refusal){
 $base="$out/sql-$script:index";++$script:index
 [IO.File]::WriteAllText("$base.sql",$sql+"`nSELECT '__SHARASDAL_ASCENT_EOF__';`n")
 $prior=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile "$base.sql" -OutputFile "$base.stdout" -ErrorFile "$base.stderr"
  $rows=@(Get-Content "$base.stdout" -Encoding UTF8);$errors=[string](Get-Content "$base.stderr" -Raw -Encoding UTF8)
  if($Refusal){if($errors -notmatch 'Sharasdal ascent context differs; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected ascent refusal: $base.stderr"}}
  elseif($code -or $errors){throw "Ascent SQL failed: $base.stderr"}
  if(!$rows.Count -or $rows[-1] -cne '__SHARASDAL_ASCENT_EOF__'){throw "Incomplete ascent SQL: $base.stdout"}
  return ,@($rows|Select-Object -SkipLast 1)
 }finally{$env:MYSQL_PWD=$prior}
}
function Snapshot([string]$tag){$sql='';foreach($table in $tables.Keys){$sql+="SELECT '$tag-$table',t.* FROM $table t ORDER BY $($tables[$table][1]);"};$sql+="SELECT '$tag-other',t.* FROM conditions t WHERE NOT($scope) ORDER BY $($tables.conditions[1]);";$sql}
function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
$live='';$setup=''
foreach($table in $tables.Keys){$live+="SELECT '$table',t.* FROM $table t WHERE $($tables[$table][0]) ORDER BY $($tables[$table][1]);";$setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($tables[$table][0]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"}
$before=Query $live
$setup+="DELETE FROM conditions WHERE $scope; INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionValue1,Comment) VALUES(17,0,999999,0,22,1,'unrelated spell sentinel'),(18,38821,999998,0,15,1,'same entry other spell sentinel');`n"
$export="SELECT 'row',SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY $($tables.conditions[1]);"
$rows=Query ($setup+(Snapshot 'original')+$migration+(Snapshot 'first')+$migration+(Snapshot 'second')+$export)
foreach($table in $tables.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Ascent migration not idempotent: $table"};if($table -ne 'conditions' -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Native context changed: $table"}}
if((Signature $rows 'original-other') -cne (Signature $rows 'first-other')){throw 'Unrelated conditions changed'}
$rules=@($rows|Where-Object {$_ -like "row`t*"}|ForEach-Object {$_.Substring(4)});if($rules.Count -ne 36){throw 'Expected exactly36 native ascent rules'}
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]]$rules)
$mutations=@(
 'UPDATE npc_spellclick_spells SET cast_flags=1 WHERE npc_entry=38821 AND spell_id=208580;',
 'UPDATE npc_spellclick_spells SET user_type=1 WHERE npc_entry=38821 AND spell_id=208580;',
 'UPDATE npc_spellclick_spells SET add_npc_flag=0 WHERE npc_entry=38821 AND spell_id=208580;',
 'DELETE FROM npc_spellclick_spells WHERE npc_entry=38821 AND spell_id=208580;',
 "UPDATE creature SET PhaseID='7002' WHERE guid=362075;",
 'UPDATE creature SET position_z=position_z+1 WHERE guid=362075;',
 'UPDATE spell_scene SET PlaybackFlags=17 WHERE MiscValue=1247;',
 'UPDATE spell_scene SET SceneScriptPackageID=1609 WHERE MiscValue=1247;',
 'INSERT INTO spell_scene_event(MiscValue,trigerSpell,MonsterCredit,Event)VALUES(1247,1,0,''complete'');',
 'UPDATE spell_target_position SET target_map=1 WHERE id=209992;',
 'UPDATE spell_target_position SET target_position_z=target_position_z+1 WHERE id=209992;',
 'UPDATE spell_linked_spell SET removeMask=1 WHERE spell_trigger=-208580;',
 'UPDATE spell_linked_spell SET target=5 WHERE spell_trigger=-208580;',
 'UPDATE spell_linked_spell SET actiontype=1 WHERE spell_trigger=-208580;',
 'UPDATE spell_linked_spell SET effectMask=1 WHERE spell_trigger=-208580;',
 'INSERT INTO spell_linked_spell(spell_trigger,spell_effect,type,comment)VALUES(209992,999997,0,''unexpected linked producer'');',
 'INSERT INTO spell_script_names(spell_id,ScriptName)VALUES(209992,''unexpected-script'');',
 'INSERT INTO conditions(SourceTypeOrReferenceId,SourceEntry,ConditionTypeOrReference,ConditionValue1)VALUES(13,208580,22,1600);',
 'INSERT INTO conditions(SourceTypeOrReferenceId,SourceEntry,ConditionTypeOrReference,ConditionValue1)VALUES(17,208580,15,1);'
)
foreach($change in $mutations){$refused=Query ($setup+$change+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal;foreach($table in $tables.Keys){if((Signature $refused "before-$table") -cne (Signature $refused "after-$table")){throw 'Refused ascent context mutated'}}}
foreach($change in @('DELETE FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=209992 AND ConditionTypeOrReference=36 AND ElseGroup=1;','UPDATE conditions SET ConditionValue2=4 WHERE SourceTypeOrReferenceId=18 AND SourceEntry=208580 AND ConditionTypeOrReference=44;')){
 $refused=Query ($setup+$migration+$change+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal;foreach($table in $tables.Keys){if((Signature $refused "before-$table") -cne (Signature $refused "after-$table")){throw 'Partial applied ascent profile mutated'}}
}
$after=Query $live;if(Compare-Object $before $after){throw 'Live World changed during temporary-only tests'}
if($VerifyLiveData){$actual=Query $export;$actualRules=@($actual|Where-Object {$_ -like "row`t*"}|ForEach-Object {$_.Substring(4)});if(Compare-Object $actualRules $rules){throw 'Live ascent admission not applied'}}
'PASS native MariaDB TEMP-only ascent admission: exactly36 source17/18 rules, original native camera/exit/click/destination/link/scene and all generic38821 spawns/sentinels retained, repeat idempotence,21 guarded context/partial refusals with full EOF/stderr. Live World unchanged by fixture; no automatic route, native scene playback or client traversal certification.'
