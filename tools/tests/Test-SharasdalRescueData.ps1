param([switch]$ExpectBound,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-rescue-data"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$previous=$env:LEGION_DIALOGUE_CONFIG
try{
 $env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
 & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SharasdalRescueCatalog.cpp -ProbeName SharasdalRescueCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"
 if(!$? -or $LASTEXITCODE){throw 'Native Breaking Bonds catalog failed'}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv"
foreach($row in @("PRODUCER`t208171`t0`t0","SPELL`t208171`tBreaking Bonds","EFFECT`t208171`t0`t0`t61`t0`t0`t49816`t0`t0`t1`t0`t0","RANGE`t208171`t12`t0`t0`t5`t5`t0","INTERRUPTS`t208171`t0`t45`t0`t0`t0`t0")){if($native -notcontains $row){throw "Native rescue metadata changed: $row"}}
if(@($native|Where-Object {$_ -match '^PRODUCER\t'}).Count -ne 1 -or @($native|Where-Object {$_ -match '^TRIGGER_IN\t'}).Count){throw 'Review other native49816 producers or208171 trigger sources'}
$misc=@($native|Where-Object {$_ -match '^MISC\t208171\t0\t'});if($misc.Count -ne 1){throw 'Missing default native rescue cast'}
$fields=$misc[0].Split("`t");if($fields[3] -ne '0' -or $fields[4] -ne '5000' -or ([uint32]$fields[7] -band 0x44)){throw 'Review cast versus channel timing before preserving7s minimum'}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'binding',entry,AIName,ScriptName,npcflag,npcflag2,faction,unit_flags,unit_flags2 FROM creature_template WHERE entry=104853;
SELECT 'click',npc_entry,spell_id,cast_flags,user_type,add_npc_flag FROM npc_spellclick_spells WHERE npc_entry=104853 OR spell_id=208171 ORDER BY npc_entry,spell_id;
SELECT 'script',spell_id,ScriptName FROM spell_script_names WHERE ABS(spell_id)=208171 ORDER BY spell_id,ScriptName;
SELECT 'conditions',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId=18 AND SourceGroup=104853) OR SourceEntry=208171;
SELECT 'event-script',COUNT(*) FROM event_scripts WHERE id=49816;
SELECT 'response',CreatureID,GroupID,ID,Duration,Sound,BroadcastTextID FROM creature_text WHERE CreatureID=105298 AND GroupID=5;
SELECT 'spawn',guid,id,map,PhaseId,position_z<795 FROM creature WHERE id=104853 ORDER BY guid;
SELECT 'scenario',ScenarioID,MapID,Team,Class FROM scenario_data WHERE MapID=1600;
SELECT 'EOF','sharasdal-rescue-data';
'@
[IO.File]::WriteAllText("$out/read.sql",$query);$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $sqlArguments=@("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names')
 $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $sqlArguments -InputFile "$out/read.sql" -OutputFile "$out/world.tsv" -ErrorFile "$out/world.stderr"
 if($code -or (Get-Item "$out/world.stderr").Length){throw 'Native rescue full EOF readback failed'}
}finally{$env:MYSQL_PWD=$previous}
$expected=@("binding`t104853`t`tnpc_adel_waterspeaker`t0`t0`t1665`t32768`t35653632","click`t104853`t208171`t1`t0`t1","conditions`t0","event-script`t0","response`t105298`t5`t0`t0`t0`t108225","spawn`t362078`t104853`t1600`t6120`t1","spawn`t362094`t104853`t1600`t6120`t0","scenario`t1066`t1600`t0`t7","EOF`tsharasdal-rescue-data")
if($ExpectBound){$expected+="script`t208171`tspell_sharasdal_breaking_bonds"}
if(Compare-Object $expected @(Get-Content "$out/world.tsv")){throw 'Review changed native rescue binding/caller/conditions/response/shared actor context'}
'PASS native26972+Hotfix sole208171/61/49816 producer, no incoming trigger, non-channel5s ordinary cast/range5/interrupt45; full EOF World sole NPC caller, exact lower+upper actor/phase, response, class/scenario, no event-script/condition override and requested binding state. No progression or character data written'
