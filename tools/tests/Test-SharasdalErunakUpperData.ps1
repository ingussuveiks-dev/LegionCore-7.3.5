param([string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-erunak-upper-data-check"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$prior=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SharasdalUpperIntroCatalog.cpp -ProbeName SharasdalUpperIntroCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native upper introduction catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
$native=Get-Content "$out/native.tsv" -Encoding UTF8
foreach($line in @("TREE`t47131`t47130`t1`t29795`t92`t50336","TREE`t47378`t47130`t1`t29661`t92`t50314","CONVERSATION_EFFECT`t209312`t0`t0","PRODUCERS`t1","EFFECT`t0`t0`t219`t0`t1868`t0`t18`t0","VISUAL_COUNT`t209312`t0","EOF`tupper-intro-audio-native")){if($native -notcontains $line){throw "Native upper intro context differs: $line"}}
if($native[-1] -cne "EOF`tupper-intro-audio-native" -or @($native|Where-Object {$_ -like "EFFECT`t*"}).Count -ne 1){throw 'Native conversation producer has extra effects or incomplete output'}
$voice=Get-Content "$PSScriptRoot/SharasdalUpperVoiceAudio.json" -Raw|ConvertFrom-Json
$expected=@($voice.Lines|ForEach-Object {"LINE`t$($_ -join "`t")"});if(Compare-Object $expected @($native|Where-Object {$_ -like "LINE`t*"})){throw 'Full five-line voice chain changed'}
$sql=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'actor',c.guid,c.id,c.map,c.position_x,c.position_y,c.position_z,c.PhaseID,t.ScriptName FROM creature c JOIN creature_template t ON t.entry=c.id WHERE c.guid IN(362063,362092,362093,362094) ORDER BY c.guid;
SELECT 'script-entry',entry,ScriptName FROM creature_template WHERE ScriptName='npc_erunak_stonespeaker' ORDER BY entry;
SELECT 'path',id,point,position_x,position_y,position_z,delay FROM waypoint_data_script WHERE id=10993811 ORDER BY point;
SELECT 'line',entry,id,textId,unk1,unk2,idx FROM conversation_data WHERE entry=1868 ORDER BY idx;
SELECT 'creature',entry,id,creatureId,creatureGuid,unk1,unk2,duration FROM conversation_creature WHERE entry=1868 ORDER BY id;
SELECT 'actor-overlay',COUNT(*) FROM conversation_actor WHERE entry=1868;
SELECT 'spell-script',spell_id,ScriptName FROM spell_script_names WHERE spell_id=209312;
SELECT 'linked',spell_trigger,spell_effect,type FROM spell_linked_spell WHERE ABS(spell_trigger)=209312 OR ABS(spell_effect)=209312;
SELECT 'spell-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN(13,17) AND SourceEntry=209312;
SELECT 'other-producer',COUNT(*) FROM smart_scripts WHERE action_type=206 AND action_param1=1868;
SELECT 'scenario',ScenarioID,MapID,Team,Class FROM scenario_data WHERE MapID=1600;
SELECT 'EOF','erunak-upper-data';
'@
[IO.File]::WriteAllText("$out/read.sql",$sql)
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$prior=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$argumentsSql=@("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names');$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $argumentsSql -InputFile "$out/read.sql" -OutputFile "$out/world.tsv" -ErrorFile "$out/world.stderr";if($code -or (Get-Item "$out/world.stderr").Length){throw 'Full-EOF native upper World read failed'}}finally{$env:MYSQL_PWD=$prior}
$expected=@'
actor	362063	102826	1600	-619.811	804.37	245.375	6120	npc_erunak_stonespeaker
actor	362092	105298	1600	-181.168	801.922	796.974	6120	npc_grash
actor	362093	102826	1600	-189.884	805.469	797.067	6120	npc_erunak_stonespeaker
actor	362094	104853	1600	-188.286	799.17	796.543	6120	npc_adel_waterspeaker
script-entry	102826	npc_erunak_stonespeaker
path	10993811	1	-172.713	802.393	796.649	3000
path	10993811	2	-155.856	802.653	796.467	0
line	1868	3961	0	0	0	0
line	1868	3962	6400	0	1	1
line	1868	8701	14733	0	1	2
line	1868	8702	28691	0	0	3
line	1868	8703	30866	0	1	4
creature	1868	0	104853	7195931	0	0	7800
creature	1868	1	102826	7195931	0	0	35016
actor-overlay	0
spell-condition	0
other-producer	0
scenario	1066	1600	0	7
EOF	erunak-upper-data
'@
if(Compare-Object ($expected.Replace("`r",'') -split "`n") @(Get-Content "$out/world.tsv")){throw 'Review changed original roles/native path/1868 payload or spell/condition/linked context'}
'PASS fresh matching26972/read-onlyHotfix native ascent47131 separate from arrival47378, sole209312/219/1868/destination-caster18/no visuals; full native five-line chain and actual World04 two-speaker47816 budget. Exact upper3/lower role, original two-node path and scenario class/map; no spell scripts/linked/13-or17 conditions/other206 producer. Full SQL EOF/empty stderr, no mutation or actual client route/voice claim.'
