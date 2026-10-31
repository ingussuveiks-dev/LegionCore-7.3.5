param([string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-kraliss-data"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$previous=$env:LEGION_DIALOGUE_CONFIG
try{
 $env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
 & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SharasdalBossCatalog.cpp -ProbeName SharasdalBossCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"
 if(!$? -or $LASTEXITCODE){throw 'Native guard catalog failed'}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$catalog=Get-Content "$out/native.tsv"
foreach($row in @("DIFFICULTY`t3491`t12`t1","TREE`t47126`t0`t4`t0`t0","TREE`t47127`t47126`t0`t1`t29660","CRITERION`t29660`t92`t49817","TREE`t47330`t47126`t0`t1`t29774","CRITERION`t29774`t92`t50240")){if($catalog -notcontains $row){throw "Matching Kraliss/junction native row changed: $row"}}
if(@($catalog|Where-Object {$_.StartsWith("PRODUCER`t")}).Count){throw 'Review alternate native61 boss criterion producer'}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'boss',c.guid,c.id,c.map,c.PhaseId,c.position_x,c.position_y,c.position_z,t.faction,t.unit_flags,t.AIName,t.ScriptName FROM creature c JOIN creature_template t ON t.entry=c.id WHERE c.id=102839 ORDER BY c.guid;
SELECT 'ally',guid,id,map,PhaseId FROM creature WHERE guid IN(362063,362066,362078) ORDER BY guid;
SELECT 'binding',entry,AIName,ScriptName FROM creature_template WHERE ScriptName='npc_kraliss' ORDER BY entry;
SELECT 'criteria-extra',COUNT(*) FROM achievement_criteria_data WHERE criteria_id=29774;
SELECT 'event-script',COUNT(*) FROM event_scripts WHERE id=50240;
SELECT 'other-205-producer',COUNT(*) FROM smart_scripts WHERE action_type=205 AND action_param1=92 AND action_param2=50240;
SELECT 'instance',map,script FROM instance_template WHERE map=1600;
SELECT 'scenario',ScenarioID,MapID,Team,Class FROM scenario_data WHERE MapID=1600;
SELECT 'EOF','sharasdal-kraliss-data';
'@
[IO.File]::WriteAllText("$out/read.sql",$query);$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3];$guardSqlArguments=@("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names')
 $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $guardSqlArguments -InputFile "$out/read.sql" -OutputFile "$out/world.tsv" -ErrorFile "$out/world.stderr"
 if($code -or (Get-Item "$out/world.stderr").Length){throw 'Guard full EOF World read failed'}
}finally{$env:MYSQL_PWD=$previous}
$expected=@("boss`t362080`t102839`t1600`t6120`t-294.227`t808.448`t260.962`t16`t32832`t`tnpc_kraliss","ally`t362063`t102826`t1600`t6120","ally`t362066`t105298`t1600`t6120","ally`t362078`t104853`t1600`t6120","binding`t102839`t`tnpc_kraliss","criteria-extra`t0","event-script`t0","other-205-producer`t0","instance`t1600`tinstance_sharasdal","scenario`t1066`t1600`t0`t7","EOF`tsharasdal-kraliss-data")
if(Compare-Object $expected @(Get-Content "$out/world.tsv")){throw 'Review changed global Kraliss spawns/binding/known lower allies/extra criterion or duplicate producer'}
'PASS fresh native26972/Hotfix root2 AND: wall49817 separate from Kraliss50240; native map3491/difficulty12 max1 so group cases defensive. Exact global102839/362080 and sole binding/native flags, known lower3 allies, native class7 map1600 instance and no alternate native61/World205/event producer/extra conditions; full EOF/stderr inspected. No SQL/character or client claim.'
