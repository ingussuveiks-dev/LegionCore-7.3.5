param([string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-guards-data"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$previous=$env:LEGION_DIALOGUE_CONFIG
try{
 $env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
 & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SharasdalGuardCatalog.cpp -ProbeName SharasdalGuardCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"
 if(!$? -or $LASTEXITCODE){throw 'Native guard catalog failed'}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$expected=@("STEP`t0`t2282`t47120","STEP`t1`t2283`t47122","STEP`t2`t2284`t47126","TREE`t47122`t0`t4`t0`t0","TREE`t47123`t47122`t0`t1`t29659","CRITERION`t29659`t92`t49816","TREE`t47197`t47122`t0`t1`t29695","CRITERION`t29695`t92`t49957")
if(Compare-Object $expected @(Get-Content "$out/native.tsv")){throw 'Review changed native1066 guard/rescue AND tree or native49957 producers'}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'guard',c.guid,c.id,c.map,c.PhaseId,c.position_x,c.position_y,c.position_z,t.AIName,t.ScriptName,t.faction FROM creature c JOIN creature_template t ON t.entry=c.id WHERE c.map=1600 AND c.id=102831 ORDER BY c.guid;
SELECT 'ally',guid,id,map,PhaseId FROM creature WHERE guid IN(362063,362066) ORDER BY guid;
SELECT 'combat',entryorguid,source_type,id,link,event_type,event_param1,event_param2,action_type,action_param1,target_type FROM smart_scripts WHERE entryorguid=102831 AND source_type=0 ORDER BY id;
SELECT 'guard-criteria-data',COUNT(*) FROM achievement_criteria_data WHERE criteria_id=29695;
SELECT 'guard-event-script',COUNT(*) FROM event_scripts WHERE id=49957;
SELECT 'other-205-producer',COUNT(*) FROM smart_scripts WHERE action_type=205 AND action_param1=92 AND action_param2=49957;
SELECT 'instance',map,script FROM instance_template WHERE map=1600;
SELECT 'scenario',ScenarioID,MapID,Team,Class FROM scenario_data WHERE MapID=1600;
SELECT 'EOF','sharasdal-guards-data';
'@
[IO.File]::WriteAllText("$out/read.sql",$query);$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3];$guardSqlArguments=@("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names')
 $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $guardSqlArguments -InputFile "$out/read.sql" -OutputFile "$out/world.tsv" -ErrorFile "$out/world.stderr"
 if($code -or (Get-Item "$out/world.stderr").Length){throw 'Guard full EOF World read failed'}
}finally{$env:MYSQL_PWD=$previous}
$expected=@("guard`t362076`t102831`t1600`t6120`t-383.491`t809.198`t248.189`tSmartAI`t`t16","ally`t362063`t102826`t1600`t6120","ally`t362066`t105298`t1600`t6120","combat`t102831`t0`t0`t0`t0`t4000`t5000`t11`t210282`t2","guard-criteria-data`t0","guard-event-script`t0","other-205-producer`t0","instance`t1600`tinstance_sharasdal","scenario`t1066`t1600`t0`t7","EOF`tsharasdal-guards-data")
if(Compare-Object $expected @(Get-Content "$out/world.tsv")){throw 'Review changed Tidecaller/known allies/SmartAI/instance/criteria context'}
'PASS native26972/read-only Hotfix1066 step1 guard49957 distinct from rescue49816 under native AND; no native61 alternate49957 producer. Full EOF World exact362076 Tidecaller, known allies, preserved210282 combat AI, binding and no duplicate producer. No SQL/character data written.'
