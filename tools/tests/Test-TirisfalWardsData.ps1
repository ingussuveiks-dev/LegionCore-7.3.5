param([string]$SqlFile='', [switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$SqlFile){$SqlFile="$repo/sql/updates/world/2026_11_17_14_tirisfal_ward_cleanse.sql"}
$out="$repo/build-extractors/runtime-test/tirisfal-wards-data"
New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $db=((Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}) -split '"')[1].Split(';')
 $filters=[ordered]@{creature_template='entry IN(103151,104583)';smart_scripts='entryorguid IN(103151,104583)';creature='id=103151';creature_template_addon='entry=103151';conditions='SourceTypeOrReferenceId=22 AND SourceEntry IN(103151,104583)'}
 $migration=Get-Content -LiteralPath $SqlFile -Raw;$setup='';$snapshot='';$prior=$env:MYSQL_PWD
 try {
  $env:MYSQL_PWD=$db[3]
  foreach($table in $filters.Keys){
   $cols=@("SHOW COLUMNS FROM $table;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names|ForEach-Object {($_ -split "`t")[0]})
   if($LASTEXITCODE -or !$cols.Count){throw 'Ward schema read failed'}
   $setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]); CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
   $pairs=($cols|ForEach-Object {"'$_',``$_``"}) -join ','
   $snapshot+="SELECT '$table',JSON_OBJECT($pairs) FROM $table WHERE $($filters[$table]) ORDER BY "+$(switch($table){'creature_template'{'entry'}'smart_scripts'{'entryorguid,source_type,id'}'creature'{'guid'}'creature_template_addon'{'entry'}default{'SourceTypeOrReferenceId,SourceEntry,SourceGroup'}})+";`n"
  }
  # Rebuild the pinned pre-repair receiver inside TEMP tables so a verification
  # after live promotion still exercises migration and every refusal branch.
  $setup+="DELETE FROM smart_scripts WHERE entryorguid=103151 AND source_type=0 AND id IN(0,1); UPDATE creature_template SET ScriptName='' WHERE entry=103151 AND ScriptName='npc_tirisfal_holy_ward';`n"
  foreach($row in (Get-Content "$PSScriptRoot/TirisfalWardOriginalReceiver.json" -Raw|ConvertFrom-Json)){
   $properties=@($row.PSObject.Properties);$columns=($properties|ForEach-Object {'`'+$_.Name+'`'}) -join ','
   $values=($properties|ForEach-Object {if($null -eq $_.Value){'NULL'}elseif($_.Value -is [string]){"CONVERT(0x$([Convert]::ToHexString([Text.Encoding]::UTF8.GetBytes($_.Value))) USING utf8mb4)"}else{[Convert]::ToString($_.Value,[Globalization.CultureInfo]::InvariantCulture)}}) -join ','
   # Empty strings have no hex bytes; represent them directly.
   $values=$values.Replace('CONVERT(0x USING utf8mb4)',"''")
   $setup+="INSERT INTO smart_scripts ($columns) VALUES ($values);`n"
  }
  $cases=[ordered]@{repaired='';'foreign-node'='INSERT INTO smart_scripts SELECT entryorguid,source_type,2,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment FROM smart_scripts WHERE entryorguid=103151 AND id=1;';'foreign-summoned'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=103151 AND id=4;';'foreign-ritualist'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=104583 AND id=2;';'foreign-template'="UPDATE creature_template SET ScriptName='foreign' WHERE entry=103151;";'foreign-addon'="UPDATE creature_template_addon SET auras='' WHERE entry=103151;";'foreign-phase'="UPDATE creature SET PhaseId='1' WHERE guid=370945;"}
  foreach($wardCase in $cases.GetEnumerator()){
   $body="$setup`n$($wardCase.Value)`nSELECT 'BEFORE';$snapshot`n$migration`nSELECT 'ONCE';$snapshot`n"
   if($wardCase.Key -eq 'repaired'){$body+="$migration`nSELECT 'TWICE';$snapshot`n"}
   $body+="SELECT 'EOF';";[IO.File]::WriteAllText("$out/$($wardCase.Key).sql",$body)
   $exit=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names','--force') -InputFile "$out/$($wardCase.Key).sql" -OutputFile "$out/$($wardCase.Key).stdout" -ErrorFile "$out/$($wardCase.Key).stderr"
   $lines=@(Get-Content "$out/$($wardCase.Key).stdout");$stderr=Get-Content "$out/$($wardCase.Key).stderr" -Raw
   if($lines[-1] -ne 'EOF'){throw 'Ward SQL capture incomplete'}
   $once=[array]::IndexOf($lines,'ONCE');$twice=[array]::IndexOf($lines,'TWICE');$before=@($lines[1..($once-1)]);$after=@($lines[($once+1)..$(if($twice -ge 0){$twice-1}else{$lines.Count-2})])
   $expected=@($before|ForEach-Object {
    $parts=$_ -split "`t",2;$table=$parts[0];$row=$parts[1]|ConvertFrom-Json
    $remove=$false
    if($wardCase.Key -eq 'repaired'){
     if($table -eq 'creature_template' -and $row.entry -eq 103151){$row.ScriptName='npc_tirisfal_holy_ward'}
     if($table -eq 'smart_scripts' -and $row.entryorguid -eq 103151 -and $row.source_type -eq 0 -and $row.id -in 0,1){$remove=$true}
    }
    if(!$remove){"$table`t$($row|ConvertTo-Json -Compress -Depth 8)"}
   })
   $actual=@($after|ForEach-Object {$parts=$_ -split "`t",2;"$($parts[0])`t$(($parts[1]|ConvertFrom-Json)|ConvertTo-Json -Compress -Depth 8)"})
   if(($actual -join "`n") -cne ($expected -join "`n")){throw "Ward full-row preservation differs: $($wardCase.Key)"}
   if($wardCase.Key -eq 'repaired'){
    if($exit -or $stderr -or $twice -lt 0 -or ($lines[($twice+1)..($lines.Count-2)] -join "`n") -cne ($after -join "`n")){throw 'Ward idempotence differs'}
   }elseif(([regex]::Matches($stderr,'ERROR 1644 \(45000\).*Tirisfal wards native graph differs')).Count -ne 1 -or ([regex]::Matches($stderr,'ERROR ')).Count -ne 1){throw 'Ward exact drift refusal differs'}
  }
  if($VerifyLiveData){
   $live=@($snapshot|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names)
   $pass=Get-Content "$out/repaired.stdout";$a=[array]::IndexOf($pass,'ONCE');$b=[array]::IndexOf($pass,'TWICE')
   if($LASTEXITCODE -or ($live -join "`n") -cne ($pass[($a+1)..($b-1)] -join "`n")){throw 'Ward live state differs'}
  }
 } finally {$env:MYSQL_PWD=$prior}
 'PASS ward migration twice, full five-table/all-field preservation and six drift refusals. Original ritualist credit/DATA, summoned sparkle, spawns and addon retained; only tested TEMP mutations.'
}
