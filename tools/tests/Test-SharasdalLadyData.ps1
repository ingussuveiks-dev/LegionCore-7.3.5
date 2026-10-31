param([string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-lady-data"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$previous=$env:LEGION_DIALOGUE_CONFIG
try{
 $env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
 & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SharasdalBossCatalog.cpp -ProbeName SharasdalBossCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"
 if(!$? -or $LASTEXITCODE){throw 'Native guard catalog failed'}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$catalog=Get-Content "$out/native.tsv"
foreach($row in @("DIFFICULTY`t3491`t12`t1","TREE`t47132`t0`t4`t0`t0","TREE`t47133`t47132`t0`t1`t29662","CRITERION`t29662`t92`t49819","TREE`t47380`t47132`t0`t1`t29803","CRITERION`t29803`t92`t50397")){if($catalog -notcontains $row){throw "Matching Kraliss/junction native row changed: $row"}}
if(@($catalog|Where-Object {$_.StartsWith("PRODUCER`t")}).Count){throw 'Review alternate native61 boss criterion producer'}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'boss',c.guid,c.id,c.map,c.PhaseId,c.position_x,c.position_y,c.position_z,t.faction,t.unit_flags,t.AIName,t.ScriptName FROM creature c JOIN creature_template t ON t.entry=c.id WHERE c.id=104856 ORDER BY c.guid;
SELECT 'ally',guid,id,map,PhaseId FROM creature WHERE guid IN(362092,362093,362094) ORDER BY guid;
SELECT 'binding',entry,AIName,ScriptName FROM creature_template WHERE ScriptName='npc_lady_zitrin' ORDER BY entry;
SELECT 'criteria-extra',COUNT(*) FROM achievement_criteria_data WHERE criteria_id=29803;
SELECT 'event-script',COUNT(*) FROM event_scripts WHERE id=50397;
SELECT 'other-205-producer',COUNT(*) FROM smart_scripts WHERE action_type=205 AND action_param1=92 AND action_param2=50397;
SELECT 'instance',map,script FROM instance_template WHERE map=1600;
SELECT 'scenario',ScenarioID,MapID,Team,Class FROM scenario_data WHERE MapID=1600;
SELECT 'EOF','sharasdal-lady-data';
'@
[IO.File]::WriteAllText("$out/read.sql",$query);$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3];$guardSqlArguments=@("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names')
 $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $guardSqlArguments -InputFile "$out/read.sql" -OutputFile "$out/world.tsv" -ErrorFile "$out/world.stderr"
 if($code -or (Get-Item "$out/world.stderr").Length){throw 'Guard full EOF World read failed'}
}finally{$env:MYSQL_PWD=$previous}
$expected=@("boss`t362091`t104856`t1600`t6120`t-21.1371`t802.283`t807.444`t16`t0`t`tnpc_lady_zitrin","boss`t362095`t104856`t1600`t6120`t50.8681`t803.073`t805.875`t16`t0`t`tnpc_lady_zitrin","ally`t362092`t105298`t1600`t6120","ally`t362093`t102826`t1600`t6120","ally`t362094`t104853`t1600`t6120","binding`t104856`t`tnpc_lady_zitrin","criteria-extra`t0","event-script`t0","other-205-producer`t0","instance`t1600`tinstance_sharasdal","scenario`t1066`t1600`t0`t7","EOF`tsharasdal-lady-data")
if(Compare-Object $expected @(Get-Content "$out/world.tsv")){throw 'Review changed global Kraliss spawns/binding/known lower allies/extra criterion or duplicate producer'}
'PASS fresh native26972/Hotfix1066 root4: separate final wall49819 and Lady50397; native difficulty cap1/group tests defensive. Exact real362095 vs early illusion362091/same shared binding and upper3 ally spawns, native class7/map1600 instance, no alternate native61/World205/event producer/extra criterion; full EOF/stderr. No SQL/character or actual combat/surrender/dialogue/client claim.'
