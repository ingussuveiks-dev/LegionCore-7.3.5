$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$sql=@"
CREATE TEMPORARY TABLE saved_character_queststatus_objectives LIKE character_queststatus_objectives; CREATE TEMPORARY TABLE character_queststatus_objectives LIKE saved_character_queststatus_objectives;
CREATE TEMPORARY TABLE saved_character_queststatus LIKE character_queststatus; CREATE TEMPORARY TABLE character_queststatus LIKE saved_character_queststatus;
CREATE TEMPORARY TABLE saved_character_queststatus_rewarded LIKE character_queststatus_rewarded; CREATE TEMPORARY TABLE character_queststatus_rewarded LIKE saved_character_queststatus_rewarded;
INSERT INTO character_queststatus_objectives VALUES (1,1,39384,0,1),(1,1,39384,1,0),(2,2,40573,0,1),(2,2,40573,3,1),(2,2,40573,2,1),(3,3,38582,2,37),(4,4,12345,0,8);
INSERT INTO character_queststatus(guid,account,quest,status) VALUES (3,3,38582,1),(5,5,38582,1),(4,4,12345,1);
INSERT INTO character_queststatus_rewarded VALUES (5,5,38582);
"@
$sql+=Get-Content "$repo/sql/updates/characters/2026_10_10_00_valsharah_objective_storage.sql" -Raw
$sql+="SELECT guid,quest,objective,data FROM character_queststatus_objectives ORDER BY guid,quest,objective; SELECT guid,status FROM character_queststatus ORDER BY guid;"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^CharacterDatabaseInfo\s*='}).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Valsharah character fixture failed'}}finally{$env:MYSQL_PWD=$old}
$expected=@("1`t39384`t0`t0","1`t39384`t3`t1","2`t40573`t2`t1","2`t40573`t4`t1","2`t40573`t5`t1","3`t38582`t2`t37","4`t12345`t0`t8","3`t3","4`t1","5`t1")
if(($rows -join '|') -ne ($expected -join '|')){throw "Character migration mismatch: $($rows -join '|')"}
'PASS: character counters remap without losing cage/cave progress or inventing boss kills; plant count, unrelated quests and rewarded quests retained.'
