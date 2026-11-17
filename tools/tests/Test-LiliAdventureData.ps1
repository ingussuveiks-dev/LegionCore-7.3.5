param([string]$SqlFile='', [switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$SqlFile){$SqlFile="$repo/sql/updates/world/2026_11_17_13_lili_personal_departure.sql"}
$out="$repo/build-extractors/runtime-test/lili-adventure-data/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $migration=[IO.File]::ReadAllText($SqlFile)
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $snapshot="CREATE TEMPORARY TABLE lili_shadow_smart_scripts AS SELECT * FROM smart_scripts WHERE entryorguid IN(100475,100481);`nCREATE TEMPORARY TABLE lili_shadow_creature_template AS SELECT * FROM creature_template WHERE entry IN(100475,100481,100972);`nCREATE TEMPORARY TABLE lili_shadow_creature AS SELECT * FROM creature WHERE guid IN(372111,340237,372112);`nCREATE TEMPORARY TABLE lili_shadow_creature_text AS SELECT * FROM creature_text WHERE CreatureID IN(100475,100481);`n"
 $shadow=$migration
 foreach($table in 'smart_scripts','creature_template','creature_text','creature'){$shadow=[regex]::Replace($shadow,"\b$table\b","lili_shadow_$table")}
 $reset="UPDATE lili_shadow_smart_scripts SET link=IF(id=1,7,8) WHERE entryorguid=100475 AND id IN(1,6); UPDATE lili_shadow_creature_template SET ScriptName='' WHERE entry IN(100475,100972); DELETE FROM lili_shadow_creature_text WHERE CreatureID=100475 AND GroupID=5;`n"
 $check="SELECT 'RESULT',SUM(entry=100475 AND ScriptName='npc_lili_adventure'),SUM(entry=100972 AND ScriptName='npc_lili_adventure_crane') FROM lili_shadow_creature_template; SELECT 'LINKS',COUNT(*) FROM lili_shadow_smart_scripts WHERE entryorguid=100475 AND id IN(1,6) AND link=0; SELECT 'REPLY',COUNT(*) FROM lili_shadow_creature_text WHERE CreatureID=100475 AND GroupID=5 AND BroadcastTextID=103341 AND Sound=62001;"
 foreach($case in 'fresh-idempotent','refuse-graph','refuse-father','refuse-vehicle','refuse-text'){
  $tamper=switch($case){
   'refuse-graph'{"UPDATE lili_shadow_smart_scripts SET action_param2=action_param2+1 WHERE entryorguid=100475 AND id=1;`n"}
   'refuse-father'{"UPDATE lili_shadow_creature SET PhaseId='6735' WHERE guid=340237;`n"}
   'refuse-vehicle'{"UPDATE lili_shadow_creature_template SET VehicleId=0 WHERE entry=100972;`n"}
   'refuse-text'{"UPDATE lili_shadow_creature_text SET Sound=0 WHERE CreatureID=100475 AND GroupID=4;`n"}
   default{''}
  }
  $refuse=$case -like 'refuse-*';$body=$snapshot+$reset+$tamper+$shadow+$(if(!$refuse){$shadow})+$check
  [IO.File]::WriteAllText("$out/$case.sql",$body)
  $prior=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$db[3];$argsSql=@("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names');if($refuse){$argsSql+='--force'}
   $exit=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $argsSql -InputFile "$out/$case.sql" -OutputFile "$out/$case.tsv" -ErrorFile "$out/$case.stderr"
  }finally{$env:MYSQL_PWD=$prior}
  $errors=[IO.File]::ReadAllText("$out/$case.stderr");$rows=Get-Content "$out/$case.tsv"
  if($exit){throw "Li Li SQL client failed: $case ($exit)"}
  if($refuse){
   if($errors -notmatch 'ERROR 1644 \(45000\).*Li Li departure source graph differs' -or ([regex]::Matches($errors,'ERROR ')).Count -ne 1){throw 'Li Li refusal diagnostic differs'}
   if($rows -notcontains "RESULT`t0`t0" -or $rows -notcontains "LINKS`t0" -or $rows -notcontains "REPLY`t0"){throw 'Refused migration changed data'}
  }elseif($errors.Length -or $rows -notcontains "RESULT`t1`t1" -or $rows -notcontains "LINKS`t2" -or $rows -notcontains "REPLY`t1"){throw 'Li Li migration/readback differs'}
 }
 if($VerifyLiveData){
  $read="SELECT COUNT(*)=2 FROM creature_template WHERE (entry=100475 AND ScriptName='npc_lili_adventure') OR (entry=100972 AND ScriptName='npc_lili_adventure_crane'); SELECT COUNT(*)=2 FROM smart_scripts WHERE entryorguid=100475 AND id IN(1,6) AND link=0; SELECT COUNT(*)=1 FROM creature_text WHERE CreatureID=100475 AND GroupID=5 AND BroadcastTextID=103341 AND Sound=62001; SELECT COUNT(*)=2 FROM quest_objectives WHERE QuestID=40633 AND ((ObjectID=100571 AND Flags=0) OR (ObjectID=102531 AND Flags=4));"
  [IO.File]::WriteAllText("$out/live.sql",$read);$prior=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$db[3];$exit=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names') -InputFile "$out/live.sql" -OutputFile "$out/live.tsv" -ErrorFile "$out/live.stderr"}finally{$env:MYSQL_PWD=$prior}
  if($exit -or (Get-Item "$out/live.stderr").Length -or ((Get-Content "$out/live.tsv") -join ',') -ne '1,1,1,1'){throw 'Li Li live data differs'}
 }
 'PASS Li Li SQL: fresh/idempotent migration, graph/father/vehicle refusal before writes; original SmartAI/gossip preserved; optional kite and required arrival separate. Temporary tables only for mutation tests.'
}
