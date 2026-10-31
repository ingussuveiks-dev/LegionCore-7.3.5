param([string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-brutes-data"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$previous=$env:LEGION_DIALOGUE_CONFIG
try{
 $env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
 & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SharasdalBrutesCatalog.cpp -ProbeName SharasdalBrutesCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"
 if(!$? -or $LASTEXITCODE){throw 'Native initial Brutes catalog failed'}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$expected=@("DIFFICULTY`t3491`t12`t1","TREE`t47120`t0`t4`t0`t0","TREE`t47121`t47120`t0`t1`t29658","CRITERION`t29658`t92`t49815","TREE`t47161`t47120`t0`t2`t29677","CRITERION`t29677`t92`t49861","TREE`t47180`t47120`t0`t1`t29688","CRITERION`t29688`t92`t49900","TREE`t47303`t47120`t0`t1`t29763","CRITERION`t29763`t92`t50161","LINE`t3630`t107447`t3000`t3631","TEXT`t107447`tWe mean you no harm, giant.`t","LINE`t3631`t107448`t2000`t3632","TEXT`t107448`tThey control us. They kill.`t","LINE`t3632`t107449`t3000`t0","TEXT`t107449`tShaman, speak with him. Perhaps we can gain a new ally down here.`t")
if(Compare-Object $expected @(Get-Content "$out/native.tsv")){throw 'Review changed native solo difficulty, two-Brute/root/recruit criteria,1693 speech or alternate49861 producer'}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'spawn',c.guid,c.id,c.map,c.PhaseId,c.position_x,c.position_y,c.position_z,t.faction,t.AIName,t.ScriptName,t.npcflag FROM creature c JOIN creature_template t ON t.entry=c.id WHERE c.id IN(102792,102826,105298) ORDER BY c.guid;
SELECT 'conversation-data',entry,id,textId,unk1,unk2,idx FROM conversation_data WHERE entry=1693 ORDER BY idx;
SELECT 'conversation-creature',entry,id,creatureId,creatureGuid,unk1,unk2,duration FROM conversation_creature WHERE entry=1693 ORDER BY id;
SELECT 'conversation-actor',COUNT(*) FROM conversation_actor WHERE entry=1693;
SELECT 'other-205-producer',COUNT(*) FROM smart_scripts WHERE action_type=205 AND action_param1=92 AND action_param2=49861;
SELECT 'brute-sai',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid IN(102792,-362064,-362067);
SELECT 'event-script',COUNT(*) FROM event_scripts WHERE id=49861;
SELECT 'extra',COUNT(*) FROM achievement_criteria_data WHERE criteria_id IN(29677,29763);
SELECT 'scenario',ScenarioID,MapID,Team,Class FROM scenario_data WHERE MapID=1600;
SELECT 'EOF','sharasdal-brutes-data';
'@
[IO.File]::WriteAllText("$out/read.sql",$query);$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3];$bruteSqlArguments=@("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names')
 $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $bruteSqlArguments -InputFile "$out/read.sql" -OutputFile "$out/world.tsv" -ErrorFile "$out/world.stderr"
 if($code -or (Get-Item "$out/world.stderr").Length){throw 'Brutes World full EOF read failed'}
}finally{$env:MYSQL_PWD=$previous}
$expected=@("spawn`t362063`t102826`t1600`t6120`t-619.811`t804.37`t245.375`t1665`t`tnpc_erunak_stonespeaker`t0","spawn`t362064`t102792`t1600`t6120`t-564.84`t802.635`t245.014`t16`t`t`t0","spawn`t362066`t105298`t1600`t6120`t-561.293`t807.247`t245.198`t1665`tSmartAI`tnpc_grash`t0","spawn`t362067`t102792`t1600`t6120`t-564.474`t812.128`t245.085`t16`t`t`t0","spawn`t362092`t105298`t1600`t6120`t-181.168`t801.922`t796.974`t1665`tSmartAI`tnpc_grash`t0","spawn`t362093`t102826`t1600`t6120`t-189.884`t805.469`t797.067`t1665`t`tnpc_erunak_stonespeaker`t0","conversation-data`t1693`t3630`t0`t32675`t0`t0","conversation-data`t1693`t3631`t4750`t32675`t1`t1","conversation-data`t1693`t3632`t8400`t32675`t0`t2","conversation-creature`t1693`t0`t102826`t7195110`t0`t0`t6300","conversation-creature`t1693`t1`t105298`t7195110`t0`t0`t6300","conversation-actor`t0","other-205-producer`t0","brute-sai`t0","event-script`t0","extra`t0","scenario`t1066`t1600`t0`t7","EOF`tsharasdal-brutes-data")
if(Compare-Object $expected @(Get-Content "$out/world.tsv")){throw 'Review changed native Brute pair, lower/upper actors, conversation1693 rows/durations, source producers or scenario class'}
'PASS native26972/Hotfix1066 root AND: real2/2 Brutes49861 distinct from heal/intro/recruit50161; map1600 difficulty3491/12 max1 player, so group matrices are defensive adapters. Exact1693 lines/broadcasts and World lower/upper pair/actors/offsets/6300+6300 native duration inputs/producer absence verified through EOF; no SQL/character data written.'
