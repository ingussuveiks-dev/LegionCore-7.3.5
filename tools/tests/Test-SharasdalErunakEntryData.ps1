param([string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-erunak-entry-data"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$prior=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SharasdalErunakCatalog.cpp -ProbeName SharasdalErunakCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native Erunak catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
$native=Get-Content "$out/native.tsv"
foreach($row in @("SIZE`tQuestObjective`t26`t294131","STEP`t0`t2282`t47120","STEP`t1`t2283`t47122","STEP`t2`t2284`t47126","STEP`t3`t2286`t47130","STEP`t4`t2287`t47132","STEP`t5`t2288`t47134","EOF`terunak-native")){if($native -notcontains $row){throw "Changed native Erunak context: $row"}}
$census=@($native|Where-Object {$_ -like "CENSUS`tQuestObjective`t*"});if($census.Count -ne 1 -or [int]$census[0].Split("`t")[2] -le 0 -or [int]$census[0].Split("`t")[3] -ne 0){throw 'Review changed native QuestObjective presence'}
# This extracted native table has no40341/105805 rows. It is not the source of
# this core's quest objectives: actual QuestData.cpp loads World quest_objectives.
$loader=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
if(!$loader.Contains('FROM quest_objectives ORDER BY StorageIndex ASC')){throw 'Actual World quest-objective loader changed'}
$query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'objective',ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=40341 ORDER BY ID;
SELECT 'actor',c.guid,c.id,c.map,c.position_x,c.position_y,c.position_z,c.PhaseID,CONCAT('[',t.ScriptName,']') FROM creature c JOIN creature_template t ON t.entry=c.id WHERE c.guid IN(362063,362064,362067,362093) ORDER BY c.guid;
SELECT 'script-entry',entry,ScriptName FROM creature_template WHERE ScriptName='npc_erunak_stonespeaker' ORDER BY entry;
SELECT 'scenario',ScenarioID,MapID,Team,Class FROM scenario_data WHERE MapID=1600;
SELECT 'EOF','erunak-entry-data';
'@
[IO.File]::WriteAllText("$out/read.sql",$query)
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$prior=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$argsSql=@("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names');$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $argsSql -InputFile "$out/read.sql" -OutputFile "$out/world.tsv" -ErrorFile "$out/world.stderr";if($code -or (Get-Item "$out/world.stderr").Length){throw 'Native full-EOF World entry read failed'}}finally{$env:MYSQL_PWD=$prior}
$expected=@'
objective	283383	40341	0	0	105805	1	0
objective	283384	40341	0	1	105787	1	2
objective	283385	40341	0	2	105806	1	2
objective	285665	40341	0	3	111286	1	2
actor	362063	102826	1600	-619.811	804.37	245.375	6120	[npc_erunak_stonespeaker]
actor	362064	102792	1600	-564.84	802.635	245.014	6120	[]
actor	362067	102792	1600	-564.474	812.128	245.085	6120	[]
actor	362093	102826	1600	-189.884	805.469	797.067	6120	[npc_erunak_stonespeaker]
script-entry	102826	npc_erunak_stonespeaker
scenario	1066	1600	0	7
EOF	erunak-entry-data
'@
if(Compare-Object ($expected.Replace("`r",'') -split "`n") @(Get-Content "$out/world.tsv")){throw 'Review changed World quest objectives/native roles/scenario identity'}
'PASS fresh native26972/read-onlyHotfix1066 roots, explicit QuestObjective table absence (not the runtime source), actual World loader and exact40341/105805 amount1/storage0 plus other objectives retained; lower362063/upper362093/brute362064+362067/script/class/map identities with full EOF/empty SQL stderr. No live mutation or actual client/full-chain claim.'
