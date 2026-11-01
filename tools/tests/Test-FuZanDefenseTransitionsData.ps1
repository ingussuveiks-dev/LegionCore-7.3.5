param([string]$ConditionsMigration,[string]$PhaseMigration,[switch]$VerifyLiveData,[string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/fuzan-defense-transitions-data")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out=[IO.Path]::GetFullPath($OutputDirectory)
function Migration([string]$name){$path="$repo/sql/pending/world/$name";if(!(Test-Path $path)){$path="$repo/sql/updates/world/$name"};$path}
if(!$ConditionsMigration){$ConditionsMigration=Migration '2026_11_01_08_fuzan_defense_actor_conditions.sql'}
if(!$PhaseMigration){$PhaseMigration=Migration '2026_11_01_09_fuzan_confrontation_phase_bridge.sql'}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo) -Test {
 $conditionSql=Get-Content $ConditionsMigration -Raw;$phaseSql=Get-Content $PhaseMigration -Raw
 $context=Get-Content "$PSScriptRoot/FuZanDefenseTransitionsOriginal.json" -Raw|ConvertFrom-Json;$fixture=$context.tables
 $filters=[ordered]@{};foreach($property in $context.filters.PSObject.Properties){$filters[$property.Name]=$property.Value};$orders=$context.orders
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[string]$ExpectedGuard){
  $stem="$out/sql-$script:queryIndex";++$script:queryIndex
  [IO.File]::WriteAllText("$stem.sql",$sql+"`nSELECT 'EOF','fuzan-defense-data';`n");$prior=$env:MYSQL_PWD
  try{
   $env:MYSQL_PWD=$connection[3]
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile "$stem.sql" -OutputFile "$stem.stdout" -ErrorFile "$stem.stderr"
   $rows=@(Get-Content "$stem.stdout" -Encoding UTF8);$errors=[IO.File]::ReadAllText("$stem.stderr")
   if($rows[-1] -cne "EOF`tfuzan-defense-data"){throw "Incomplete native SQL EOF: $stem"}
   if($ExpectedGuard){if($errors -notmatch ([regex]::Escape($ExpectedGuard)) -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected refusal diagnostics: $stem"}}
   elseif($code -or $errors){throw "Native defense SQL failed: $stem"}
   return ,$rows
  }finally{$env:MYSQL_PWD=$prior}
 }
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
 function Snapshot([string]$tag,[switch]$Live){$sql='';foreach($table in $filters.Keys){$where=if($Live){" WHERE $($filters[$table])"}else{''};$sql+="SELECT '$tag-$table',t.* FROM $table t$where ORDER BY $($orders.$table);`n"};$sql}
 $senders=Query "SELECT 'sender',action_param1,action_param2,COUNT(*) FROM smart_scripts WHERE action_type=45 AND target_param1=109391 AND ((action_param1=1 AND action_param2=1) OR (action_param1=2 AND action_param2=2)) GROUP BY action_param1,action_param2 ORDER BY action_param1;"
 if((Signature $senders 'sender') -cne "1`t1`t4`n2`t2`t1"){throw 'Actual global native Yu lon senders differ; investigate before shadowing'}
 $setup='';foreach($table in $filters.Keys){$setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"}
 $scope=$context.condition_scope
 $restore="UPDATE conditions SET ConditionTarget=0 WHERE $scope;UPDATE creature SET PhaseId='6905' WHERE guid=373396;`n"
 $expectedCondition="UPDATE conditions SET ConditionTarget=1 WHERE $scope;`n"
 $expectedPhase="UPDATE creature SET PhaseId='6905 6906' WHERE guid=373396;`n"
 # Certify the complete native context before restoring only the known two
 # historical fields in TEMP tables. Current/future cohort order is guarded.
 $certified=$setup+$conditionSql+$phaseSql+$restore
 $pins='';foreach($table in $filters.Keys){if(!@($fixture.$table).Count){continue};$columns=@($fixture.$table[0].PSObject.Properties.Name|ForEach-Object {'`'+$_+'`'}) -join ',';$pins+="SELECT 'pin-$table',$columns FROM $table ORDER BY $($orders.$table);`n"}
 $rows=Query ($certified+$pins+(Snapshot 'original')+$expectedCondition+(Snapshot 'expected08')+$expectedPhase+(Snapshot 'expected09')+$restore+$conditionSql+(Snapshot 'first08')+$conditionSql+(Snapshot 'second08')+$phaseSql+(Snapshot 'first09')+$phaseSql+(Snapshot 'second09')+$conditionSql+(Snapshot 'replayed08'))
 foreach($table in $filters.Keys){
  foreach($pair in @(@('expected08','first08'),@('first08','second08'),@('expected09','first09'),@('first09','second09'),@('first09','replayed08'))){if((Signature $rows "$($pair[0])-$table") -cne (Signature $rows "$($pair[1])-$table")){throw "Exact delta/idempotence differs: $table/$($pair -join ',')"}}
  if($table -ne 'conditions' -and (Signature $rows "original-$table") -cne (Signature $rows "first08-$table")){throw "Conditions migration changed unrelated native data: $table"}
  if($table -ne 'creature' -and (Signature $rows "first08-$table") -cne (Signature $rows "first09-$table")){throw "Phase migration changed unrelated native data: $table"}
  if(@($fixture.$table).Count){$pin=@($fixture.$table|ForEach-Object {($_.PSObject.Properties.Value|ForEach-Object {if($null -eq $_){'NULL'}else{[Convert]::ToString($_,[Globalization.CultureInfo]::InvariantCulture)}}) -join "`t"});if(($pin -join "`n") -cne (Signature $rows "pin-$table")){throw "Pinned full native context differs: $table"}}
 }
 $unrelated="CREATE TEMPORARY TABLE extra_spawn LIKE creature;INSERT INTO extra_spawn SELECT * FROM creature WHERE guid=373396;UPDATE extra_spawn SET guid=999999999,id=999999,map=1,PhaseId='4242';INSERT INTO creature SELECT * FROM extra_spawn;`n"
 $extra=Query ($certified+$unrelated+$expectedCondition+$expectedPhase+(Snapshot 'expected-extra')+$restore+$conditionSql+$phaseSql+(Snapshot 'actual-extra')+"SELECT 'unrelated',guid,id,map,PhaseId FROM creature WHERE guid=999999999;")
 foreach($table in $filters.Keys){if((Signature $extra "expected-extra-$table") -cne (Signature $extra "actual-extra-$table")){throw "Successful migrations changed unrelated native record: $table"}}
 if((Signature $extra 'unrelated') -cne "999999999`t999999`t1`t4242"){throw 'Unrelated actor/phase was not preserved'}
 $variants=[ordered]@{
  'custom-yulon-script'="UPDATE creature_template SET ScriptName='custom' WHERE entry=109391;"
  'wrong-yulon-ai'="UPDATE creature_template SET AIName='' WHERE entry=109391;"
  'wrong-korithis-phase'="UPDATE creature SET PhaseId='6905' WHERE guid=14670029;"
  'wrong-yulon-source-phase'="UPDATE creature SET PhaseId='6906' WHERE guid=373396;"
  'wrong-yulon-map'="UPDATE creature SET map=1 WHERE guid=373396;"
  'wrong-yulon-source-guid'="UPDATE creature SET guid=373394 WHERE guid=373396;"
  'unknown-condition-target'="UPDATE conditions SET ConditionTarget=2 WHERE $scope;"
  'changed-native-update-target'="UPDATE conditions SET ConditionTarget=1 WHERE SourceTypeOrReferenceId=22 AND SourceEntry=109391 AND SourceGroup=11;"
  'wrong-criterion'="UPDATE conditions SET ConditionValue1=49568 WHERE $scope;"
  'wrong-criterion-negation'="UPDATE conditions SET NegativeCondition=0 WHERE $scope;"
  'missing-condition'="DELETE FROM conditions WHERE $scope;"
  'custom-condition-script'="UPDATE conditions SET ScriptName='custom' WHERE $scope;"
  'wrong-first-wave'="UPDATE smart_scripts SET action_param1=110471 WHERE entryorguid=109391 AND source_type=0 AND id=0;"
  'wrong-next-wave-link'="UPDATE smart_scripts SET link=0 WHERE entryorguid=109391 AND source_type=0 AND id=8;"
  'wrong-death-credit'="UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=109438 AND source_type=0 AND id=2;"
  'wrong-credit-selector'="UPDATE smart_scripts SET target_type=18 WHERE entryorguid=109438 AND source_type=0 AND id=2;"
  'wrong-path'="UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=109391 AND source_type=0 AND id=13;"
  'wrong-path-point'="UPDATE waypoints SET position_z=position_z+1 WHERE entry=109391 AND pointid=2;"
  'wrong-dialogue-time'="UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=109821 AND source_type=0 AND id=0;"
  'wrong-reply-data'="UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=109821 AND source_type=0 AND id=1;"
  'changed-meditation'="UPDATE creature_template_addon SET auras='' WHERE entry=109391;"
  'changed-text'="UPDATE creature_text SET Text='custom' WHERE CreatureID=109821 AND GroupID=1;"
  'changed-scaling'="UPDATE creature_template_scaling SET LevelScalingMin=1 WHERE Entry=109391;"
  'changed-player-phase-gate'="UPDATE conditions SET ConditionValue2=4 WHERE SourceTypeOrReferenceId=23 AND SourceGroup=8262 AND SourceEntry=3 AND ConditionTypeOrReference=44 AND ConditionValue2=5;"
  'changed-player-phase-definition'="UPDATE phase_definitions SET phaseId='6905' WHERE zoneId=8262 AND entry=3;"
  'unknown-global-sender'="CREATE TEMPORARY TABLE extra_sender LIKE smart_scripts;INSERT INTO extra_sender SELECT * FROM smart_scripts WHERE entryorguid=109438 AND source_type=0 AND id=3;UPDATE extra_sender SET entryorguid=999999;INSERT INTO smart_scripts SELECT * FROM extra_sender;"
 }
 foreach($name in $variants.Keys){
  foreach($phase in @($false,$true)){
   $before=$certified+$(if($phase){$conditionSql}else{''})+$variants[$name]+(Snapshot 'before')
   $sql=if($phase){$phaseSql}else{$conditionSql};$guard=if($phase){'fuzan_confrontation_phase_native_guard'}else{'fuzan_defense_conditions_native_guard'}
   $bad=Query ($before+$sql+(Snapshot 'after')) $guard
   foreach($table in $filters.Keys){if((Signature $bad "before-$table") -cne (Signature $bad "after-$table")){throw "Refusal mutated native data: $name/$phase/$table"}}
  }
  "PASS both migration refusals $name"
 }
 foreach($orderCase in @(@{name='phase-before-condition';setup='';sql=$phaseSql;guard='fuzan_confrontation_phase_native_guard'},@{name='phase-first-partial';setup=$expectedPhase;sql=$conditionSql;guard='fuzan_defense_conditions_native_guard'})){
  $bad=Query ($certified+$orderCase.setup+(Snapshot 'before')+$orderCase.sql+(Snapshot 'after')) -ExpectedGuard $orderCase.guard
  foreach($table in $filters.Keys){if((Signature $bad "before-$table") -cne (Signature $bad "after-$table")){throw "Out-of-order refusal changed data: $table"}};"PASS refusal $($orderCase.name)"
 }
 if($VerifyLiveData){
  $live=Query (Snapshot 'live' -Live);foreach($table in $filters.Keys){if((Signature $live "live-$table") -cne (Signature $rows "first09-$table")){throw "Full live native transition context differs: $table"}}
  foreach($path in @($ConditionsMigration,$PhaseMigration)){
   $payload=[IO.File]::ReadAllText($path).Replace("`r`n","`n");if($payload.Contains([string][char]26)){throw 'Unexpected DOS EOF'};$sha=[Security.Cryptography.SHA1]::Create()
   try{$hash=[Convert]::ToHexString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($payload)))}finally{$sha.Dispose()}
   $history=Query "SELECT 'updater',hash,state FROM updates WHERE name='$([IO.Path]::GetFileName($path))';"
   if((Signature $history 'updater') -cne "$hash`tRELEASED"){throw 'Actual updater hash/state differs'}
  }
 }
 'PASS native TEMP12: exact one DATA ConditionTarget plus one source PhaseId delta, native UPDATE target0 retained; idempotence/cohort order/54 refusals and optional full live equality/updater hashes'
}
