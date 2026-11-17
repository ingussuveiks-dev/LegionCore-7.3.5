param([switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/titanstrike-fusion-data"
New-Item -ItemType Directory -Force $out|Out-Null
$migrationPath="$repo/sql/updates/world/2026_11_17_12_titanstrike_fusion_receipts.sql"
if(!(Test-Path $migrationPath)){$migrationPath="$repo/sql/pending/2026_11_17_12_titanstrike_fusion_receipts.sql"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $settings=((Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}) -split '"')[1].Split(';')
 $filters=[ordered]@{
  creature_template='entry IN(106558,106613,106925)'
  gameobject_template='entry IN(249765,249773)'
  smart_scripts='entryorguid IN(106558,10655800,106613,106925,249765,249773)'
  conditions='((SourceTypeOrReferenceId=22 AND SourceEntry IN(249765,249773)) OR (SourceTypeOrReferenceId=15 AND SourceGroup=19589))'
 }
 $migration=Get-Content -LiteralPath $migrationPath -Raw;$setup='';$snapshot='';$prior=$env:MYSQL_PWD
 try {
  $env:MYSQL_PWD=$settings[3]
  foreach($table in $filters.Keys){
   $columns=@("SHOW COLUMNS FROM $table;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($settings[0]) --port=$($settings[1]) --user=$($settings[2]) --database=$($settings[4]) --batch --raw --skip-column-names|ForEach-Object {($_ -split "`t")[0]})
   if($LASTEXITCODE -or !$columns.Count){throw 'Native fusion schema read failed'}
   $setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"
   $pairs=($columns|ForEach-Object {"'$_',``$_``"}) -join ','
   $snapshot+="SELECT '$table',JSON_OBJECT($pairs) FROM $table WHERE $($filters[$table]) ORDER BY "+$(if($table -eq 'smart_scripts'){'entryorguid,source_type,id'}elseif($table -eq 'conditions'){'SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference'}else{'entry'})+";`n"
  }
  $setup+=@"
UPDATE creature_template SET ScriptName='' WHERE ScriptName IN('npc_titanstrike_fusion_mimiron','npc_titanstrike_fusion_core');
UPDATE conditions SET ConditionTypeOrReference=29,ConditionTarget=0,ConditionValue1=IF(SourceEntry=249765,106613,106925),ConditionValue2=50,NegativeCondition=1,ScriptName='' WHERE ScriptName='condition_titanstrike_fusion_ready';
UPDATE smart_scripts SET link=2 WHERE entryorguid IN(106613,106925) AND source_type=0 AND id=1 AND link=0;
"@
  $cases=[ordered]@{
   repaired=''
   foreign_actor="UPDATE creature_template SET ScriptName='custom' WHERE entry=106558;"
   foreign_timer='UPDATE smart_scripts SET event_param3=7000 WHERE entryorguid=106613 AND id=0;'
   foreign_link='UPDATE smart_scripts SET link=3 WHERE entryorguid=106925 AND id=1;'
   foreign_node='INSERT INTO smart_scripts SELECT entryorguid,source_type,2,0,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment FROM saved_smart_scripts WHERE entryorguid=106613 AND source_type=0 AND id=1;'
   foreign_owner='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=106558 AND source_type=0 AND id=4;'
   foreign_panel="UPDATE gameobject_template SET ScriptName='custom' WHERE entry=249765;"
   foreign_condition='UPDATE conditions SET ConditionValue2=100 WHERE SourceTypeOrReferenceId=22 AND SourceEntry=249773;'
   foreign_quest='UPDATE conditions SET ConditionValue1=1 WHERE SourceTypeOrReferenceId=15;'
  }
  foreach($fusionCase in $cases.GetEnumerator()){
   $sql="$setup`n$($fusionCase.Value)`nSELECT 'BEFORE';$snapshot`n$migration`nSELECT 'ONCE';$snapshot`n"
   if($fusionCase.Key -eq 'repaired'){$sql+="$migration`nSELECT 'TWICE';$snapshot`n"}
   $sql+="SELECT 'EOF';";[IO.File]::WriteAllText("$out/$($fusionCase.Key).sql",$sql)
   $exit=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($settings[0])","--port=$($settings[1])","--user=$($settings[2])","--database=$($settings[4])",'--batch','--raw','--skip-column-names','--force') -InputFile "$out/$($fusionCase.Key).sql" -OutputFile "$out/$($fusionCase.Key).stdout" -ErrorFile "$out/$($fusionCase.Key).stderr"
   $rows=@(Get-Content "$out/$($fusionCase.Key).stdout");$stderr=Get-Content "$out/$($fusionCase.Key).stderr" -Raw
   if($rows[-1] -ne 'EOF'){throw 'Fusion SQL output was incomplete'}
   $start=[array]::IndexOf($rows,'BEFORE');$once=[array]::IndexOf($rows,'ONCE');$twice=[array]::IndexOf($rows,'TWICE')
   if($start -ne 0 -or $once -le $start){throw 'Fusion snapshots missing'}
   $before=@($rows[1..($once-1)]);$after=@($rows[($once+1)..$(if($twice -ge 0){$twice-1}else{$rows.Count-2})])
   $expected=@($before|ForEach-Object {
    $parts=$_ -split "`t",2;$table=$parts[0];$row=$parts[1]|ConvertFrom-Json
    if($fusionCase.Key -eq 'repaired'){
     if($table -eq 'creature_template'){$row.ScriptName=if($row.entry -eq 106558){'npc_titanstrike_fusion_mimiron'}else{'npc_titanstrike_fusion_core'}}
     if($table -eq 'smart_scripts' -and $row.entryorguid -in 106613,106925 -and $row.source_type -eq 0 -and $row.id -eq 1){$row.link=0}
     if($table -eq 'conditions' -and $row.SourceTypeOrReferenceId -eq 22){$row.ConditionTypeOrReference=32;$row.ConditionTarget=1;$row.ConditionValue1=32;$row.ConditionValue2=0;$row.NegativeCondition=0;$row.ScriptName='condition_titanstrike_fusion_ready'}
    }
    "$table`t$($row|ConvertTo-Json -Compress -Depth 8)"
   })
   $actual=@($after|ForEach-Object {$parts=$_ -split "`t",2;"$($parts[0])`t$(($parts[1]|ConvertFrom-Json)|ConvertTo-Json -Compress -Depth 8)"})
   if(($actual -join "`n") -cne ($expected -join "`n")){throw "Fusion full-row preservation differs: $($fusionCase.Key)"}
   if($fusionCase.Key -eq 'repaired'){
    if($exit -or $stderr -or $twice -lt 0 -or ($rows[($twice+1)..($rows.Count-2)] -join "`n") -cne ($after -join "`n")){throw 'Fusion idempotent shadow differs'}
   }elseif(@([regex]::Matches($stderr,'ERROR 1644 \(45000\).*Titanstrike fusion graph differs')).Count -ne 1){throw 'Foreign fusion scope did not emit its expected refusal'}
  }
  if($VerifyLiveData){
   $sql="SELECT 'BEFORE';$snapshot`n$migration`nSELECT 'AFTER';$snapshot`nSELECT 'EOF';"
   # Verification of the real deployed state is read-only; replay above uses
   # TEMP shadows exclusively. Do not execute the migration on live tables.
   $live=@("$snapshot"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($settings[0]) --port=$($settings[1]) --user=$($settings[2]) --database=$($settings[4]) --batch --raw --skip-column-names)
   $pass=@(Get-Content "$out/repaired.stdout");$a=[array]::IndexOf($pass,'ONCE');$b=[array]::IndexOf($pass,'TWICE');$target=@($pass[($a+1)..($b-1)])
   if($LASTEXITCODE -or ($live -join "`n") -cne ($target -join "`n")){throw 'Live fusion data differs from the complete tested shadow'}
  }
 } finally {$env:MYSQL_PWD=$prior}
 'PASS fusion migration twice, full four-table/all-field preservation, eight exact foreign-scope refusals; no live records changed by fixture'
}
