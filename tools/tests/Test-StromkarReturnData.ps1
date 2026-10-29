param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-return-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_29_07_stromkar_return_gaze.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_29_07_stromkar_return_gaze.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $prior=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  try{$env:MYSQL_PWD=$connection[3];$ErrorActionPreference='Continue'
   $rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names --force 2>$errorFile)
   $code=$LASTEXITCODE;$ErrorActionPreference='Stop';[IO.File]::WriteAllText("$errorFile.exit.txt",[string]$code);$errors=[string](Get-Content $errorFile -Raw)
   if($Refusal){if($errors -notmatch 'Stromkar native return actor/area/actions/conditions differ; review before promotion' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Return SQL failed: $errorFile"};return ,$rows
  }finally{$env:MYSQL_PWD=$prior}
 }
 $filters=[ordered]@{creature_template='entry IN(104307,93819,93823)';smart_scripts='entryorguid IN(104307,93819,93823)';conditions='SourceTypeOrReferenceId=26 AND SourceGroup IN(11869,16555)';areatrigger_template='customEntry IN(11869,16555)';areatrigger_actions='customEntry IN(11869,16555)'}
 $order=@{creature_template='entry';smart_scripts='entryorguid,source_type,id,link';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget';areatrigger_template='entry,customEntry';areatrigger_actions='entry,customEntry,id'}
 $setup='';foreach($table in $filters.Keys){$setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"}
 $baseline=@'
UPDATE creature_template SET ScriptName='' WHERE entry=93819;
DELETE FROM smart_scripts WHERE entryorguid=104307 AND source_type=0 AND id=6;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=26 AND SourceGroup=11869;
'@
 function Snapshot([string]$name){$sql='';foreach($table in $filters.Keys){$sql+="SELECT '$name-$table',t.* FROM $table t ORDER BY $($order[$table]);"};return $sql}
 function Signature($rows,[string]$name){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$name`t*"}|ForEach-Object {$_.Substring($name.Length+1)}))}
 $rows=Query ($setup+$migration+$baseline+(Snapshot 'before')+$migration+(Snapshot 'first')+$migration+(Snapshot 'second')+"SELECT 'rules',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,IF(ScriptName='','-',ScriptName) FROM conditions WHERE SourceTypeOrReferenceId=26 AND SourceGroup=11869 ORDER BY ElseGroup,ConditionTypeOrReference,ConditionTarget;SELECT 'ghost',id,link,event_type,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,target_type FROM smart_scripts WHERE entryorguid=104307 ORDER BY id;")
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Return not idempotent: $table"}}
 foreach($table in @('areatrigger_template','areatrigger_actions')){if((Signature $rows "before-$table") -cne (Signature $rows "first-$table")){throw "Native shared area changed: $table"}}
 $roundtrip=Query ($setup+$baseline+(Snapshot 'before')+$migration+$baseline+(Snapshot 'after'))
 foreach($table in $filters.Keys){if((Signature $roundtrip "before-$table") -cne (Signature $roundtrip "after-$table")){throw "Unrelated return metadata changed: $table"}}
 [IO.File]::WriteAllLines("$out/temp.tsv",[string[]]$rows)
 [IO.File]::WriteAllLines("$out/rules.tsv",[string[]]@($rows|Where-Object {$_ -like "rules`t*"}|ForEach-Object {$_.Substring(6)}))
 [IO.File]::WriteAllLines("$out/ghost.tsv",[string[]]@($rows|Where-Object {$_ -like "ghost`t*"}|ForEach-Object {$_.Substring(6)}))
 if(@(Get-Content "$out/rules.tsv").Count -ne 21){throw 'Expected21 native return conditions'}
 $variants=@{
 'ghost-script'="UPDATE creature_template SET ScriptName='custom' WHERE entry=104307;"
 'valkyr-script'="UPDATE creature_template SET ScriptName='custom' WHERE entry=93819;"
 'valkyr-reset'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=93819 AND id=0;'
 'ghost-speech'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=104307 AND id=4;'
 'ghost-extra'="INSERT INTO smart_scripts(entryorguid,source_type,id,comment)VALUES(104307,0,99,'custom');"
 'area-visual'='UPDATE areatrigger_template SET VisualID=1 WHERE entry=0 AND customEntry=11869;'
 'area-radius'='UPDATE areatrigger_template SET Radius=6 WHERE entry=0 AND customEntry=11869;'
 'area-action'='UPDATE areatrigger_actions SET targetFlags=1 WHERE entry=0 AND customEntry=11869 AND id=0;'
 'partial-rule'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference)VALUES(26,11869,0,36);"
 'applied-script'="UPDATE creature_template SET ScriptName='' WHERE entry=93819;"
 'applied-rule'='DELETE FROM conditions WHERE SourceTypeOrReferenceId=26 AND SourceGroup=11869 AND ElseGroup=0;'
 'applied-signal'='DELETE FROM smart_scripts WHERE entryorguid=104307 AND id=6;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$check=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $check "before-$table") -cne (Signature $check "after-$table")){throw "Custom state overwritten: $name"}}
  [IO.File]::WriteAllLines("$out/refusal-$name.tsv",[string[]]$check)
 }
 if($VerifyLiveData){$sha=(Get-FileHash $MigrationFile -Algorithm SHA1).Hash;$live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_29_07_stromkar_return_gaze.sql';SELECT 'binding',ScriptName FROM creature_template WHERE entry=93819;SELECT 'rules',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=26 AND SourceGroup=11869;"
  foreach($line in @("release`t$sha`tRELEASED","binding`tnpc_stromkar_valkyr","rules`t21")){if($live -notcontains $line){throw "Live return state differs: $line"}}
 }
 'PASS native return full-schema TEMP repeat/roundtrip,12 custom-state refusals,21 rules and shared Gaze/Flame/Danica preservation. No live metadata changes by fixture; native area/action and client checks remain separate.'
}
