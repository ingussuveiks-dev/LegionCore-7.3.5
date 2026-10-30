param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/blood-prison-guard-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_26_blood_prison_guard.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_26_blood_prison_guard.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql+"`nSELECT '__BLOOD_GUARD_SQL_COMPLETE__';`n")
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];$sqlFile="$out/sql-$($script:queryIndex-1).sql";$stdoutFile="$out/sql-$($script:queryIndex-1).stdout"
   # Preserve complete native stdout/stderr bytes, including the tail after an
   # expected --force error; a PowerShell native pipeline lost part of a row.
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile $sqlFile -OutputFile $stdoutFile -ErrorFile $errorFile
   $rows=@(Get-Content $stdoutFile -Encoding UTF8);$errors=[string](Get-Content $errorFile -Raw -Encoding UTF8)
   if($Refusal){if($errors -notmatch 'Blood prison guard/context differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Blood prison guard refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Blood prison guard SQL failed: $errorFile"}
   if(!$rows.Count -or $rows[-1] -cne '__BLOOD_GUARD_SQL_COMPLETE__'){throw "Incomplete native SQL output: $stdoutFile"}
   return ,@($rows|Select-Object -SkipLast 1)
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/BloodPrisonGuardOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='source_type IN(0,9) AND entryorguid IN(101539,10153900,102244,102245,102304,102362,102955,102561)';conditions='(SourceTypeOrReferenceId=22 AND SourceEntry IN(101539,10153900,102244,102245,102304,102362,102955,102561)) OR(SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(203481,203480,202807,48707,205660,203373,183345,202177,182217))';creature_text='CreatureID IN(101539,102244,102245,102304,102362,102955,102561)';creature_template='entry IN(101539,102244,102245,102304,102362,102955,102561)';creature_template_wdb='Entry IN(101539,102244,102245,102304,102362,102955,102561)';creature='id IN(101539,102244,102245,102304,102362,102955,102561)';waypoints='entry IN(101539,10153900,10153901,10153902,10153903,10153904,10153905,10153906,10153907,10153908,10153909,10153910,102244)';npc_spellclick_spells='npc_entry IN(102955,102561)';gameobject_template='entry IN(246565,246478,245854)';gameobject='id IN(246565,246478,245854)';spell_script_names='spell_id IN(203481,203480,202807,48707,205660,203373,183345,202177,182217)';scenario_data='MapID=1489';phase_definitions='phaseId=''5955'''}
 $order=@{smart_scripts='entryorguid,source_type,id,link';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2';creature_text='CreatureID,GroupID,ID';creature_template='entry';creature_template_wdb='Entry';creature='guid';waypoints='entry,pointid';npc_spellclick_spells='npc_entry,spell_id';gameobject_template='entry';gameobject='guid';spell_script_names='spell_id,ScriptName';scenario_data='ScenarioID,MapID,DifficultyID,Team,Class,LfgDungeonID';phase_definitions='zoneId,entry'}
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
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Not idempotent $table"};if($table -ne 'conditions' -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Native return data changed $table"}}
 $original=@((Signature $rows 'original-conditions').Split("`n"));$applied=@((Signature $rows 'first-conditions').Split("`n"))
 if($original.Count -ne 4 -or $applied.Count -ne $original.Count){throw 'Missing native Blood prison condition context'}
 $edits=0;for($i=0;$i -lt $original.Count;++$i){$f=$original[$i].Split("`t")
  if($f[0] -eq '22' -and $f[1] -eq '4' -and $f[2] -eq '102244' -and $f[5] -eq '29'){
   if($f[3] -ne '0' -or $f[4] -ne '0' -or $f[6] -ne '0' -or $f[7] -ne '102244' -or $f[8] -ne '100' -or $f[9] -ne '0' -or $f[10] -ne '1'){throw 'Original native guard anchor differs'};$f[6]='1';++$edits
  }
  if(($f -join "`t") -cne $applied[$i]){throw 'Unexpected guard/context condition field change'}
 };if($edits -ne 1){throw 'Expected exactly one native condition target edit'}
 foreach($pair in @(@('original','baseline-conditions'),@('first','conditions'))){$payload=@((Signature $rows "$($pair[0])-conditions").Split("`n")|ForEach-Object {$f=$_.Split("`t");if($f[0] -eq '22' -and $f[1] -eq '4' -and $f[2] -eq '102244'){($f[1],$f[5],$f[6],$f[7],$f[8],$f[9],$f[10]) -join "`t"}});if($payload.Count -ne 2){throw 'Missing native stage3 guard conditions'};[IO.File]::WriteAllLines("$out/$($pair[1]).tsv",[string[]]$payload)}
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original);[IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$applied)
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'native-neighbour'='UPDATE conditions SET ConditionValue1=102245 WHERE SourceEntry=102244 AND SourceGroup=4 AND ConditionTypeOrReference=29;';
 'native-radius'='UPDATE conditions SET ConditionValue2=99 WHERE SourceEntry=102244 AND SourceGroup=4 AND ConditionTypeOrReference=29;';
 'native-negative'='UPDATE conditions SET NegativeCondition=0 WHERE SourceEntry=102244 AND SourceGroup=4 AND ConditionTypeOrReference=29;';
 'native-step'='UPDATE conditions SET ConditionValue2=4 WHERE SourceEntry=102244 AND SourceGroup=4 AND ConditionTypeOrReference=44;';
 'native-other-stage'='UPDATE conditions SET ConditionValue2=4 WHERE SourceEntry=102561 AND SourceGroup=7;';
 'native-death'='UPDATE smart_scripts SET event_type=4 WHERE entryorguid=102244 AND source_type=0 AND id=3;';
 'native-data'='UPDATE smart_scripts SET action_param1=4 WHERE entryorguid=102244 AND source_type=0 AND id=3;';
 'native-recipient'='UPDATE smart_scripts SET target_param2=31 WHERE entryorguid=102244 AND source_type=0 AND id=3;';
 'native-guide-credit'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=101539 AND source_type=0 AND id=25;';
 'native-key-summon'='UPDATE smart_scripts SET target_type=130 WHERE entryorguid=102304 AND source_type=0 AND id=5;';
 'native-key-click'='UPDATE npc_spellclick_spells SET cast_flags=0 WHERE npc_entry=102955 AND spell_id=203480;';
 'native-key-delay'='UPDATE smart_scripts SET event_param1=1 WHERE entryorguid=102955 AND source_type=9 AND id=0;';
 'native-path'='UPDATE waypoints SET position_x=position_x+1 WHERE entry=10153903 AND pointid=1;';
 'native-door'='UPDATE gameobject_template SET Data0=Data0+1 WHERE entry=246565;';
 'native-door-spawn'='UPDATE gameobject SET phaseMask=2 WHERE id=246565;';
 'native-ai'="UPDATE creature_template SET ScriptName='custom' WHERE entry=102244;";
 'native-phase'="UPDATE phase_definitions SET phaseId='1' WHERE phaseId='5955';";
 'native-route'='UPDATE scenario_data SET Class=1 WHERE MapID=1489 AND ScenarioID=940;';
 'custom-condition'='INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,4,102244,9,1);';
 'applied-anchor'='UPDATE conditions SET ConditionTarget=2 WHERE SourceEntry=102244 AND SourceGroup=4 AND ConditionTypeOrReference=29;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom Blood prison guard data mutated $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_26_blood_prison_guard.sql' AND hash='$hash' AND state='RELEASED';";if($live -notcontains '1'){throw 'Live Blood prison guard migration/hash differs'}
  $live=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $live "live-$table") -cne (Signature $rows "first-$table")){throw "Live native Blood prison guard context differs $table"}}
 }
 'PASS full13table Blood prison guard: exactly1 condition target0->source1; native940/3,last alive guard100yd,guide/data/57pathpoints/key203481+203480/doors/phase/context unchanged; twice-idempotent/20 custom refusals/live when requested'
}
