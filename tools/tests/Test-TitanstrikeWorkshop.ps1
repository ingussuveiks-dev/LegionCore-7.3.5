param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/titanstrike-workshop"
New-Item -ItemType Directory -Force $out | Out-Null
$fixture=Get-Content "$PSScriptRoot/TitanstrikeRunesTest.cpp" -Raw
$a=$fixture.IndexOf('#include "TitanstrikeActors.inc"')
[IO.File]::WriteAllText("$out/TitanstrikeWorkshopFixture.inc",$fixture.Substring(0,$a))
$source=Get-Content "$repo/src/server/scripts/Legion/titanstrike_workshop.cpp" -Raw
$a=$source.IndexOf('struct npc_titanstrike_xr947');$b=$source.IndexOf('void AddSC_titanstrike_workshop()',$a)
[IO.File]::WriteAllText("$out/TitanstrikeWorkshopActor.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" /I`"$repo/src/server/scripts/Legion`" `"$PSScriptRoot/TitanstrikeWorkshopTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Production workshop callback regression failed'}
$migration=Get-Content "$repo/sql/updates/world/2026_10_23_05_titanstrike_workshop.sql" -Raw
$sql=''
foreach($table in @('creature_template','smart_scripts','conditions','conversation_data','conversation_creature')){
 $where=switch($table){'creature_template'{'entry=105016'}'smart_scripts'{'entryorguid IN (105016,104904,10490400,106352,106354,106376,106428)'}'conditions'{'SourceTypeOrReferenceId=22 AND SourceEntry=105016'}default{'entry IN (2055,9002055)'}}
 $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table;`nINSERT INTO saved_$table SELECT * FROM $table WHERE $where;`nCREATE TEMPORARY TABLE $table LIKE saved_$table;`nINSERT INTO $table SELECT * FROM saved_$table;`n"
}
$snapshot=@'
SELECT 'sai',entryorguid,source_type,id,link,event_type,event_param1,event_param2,action_type,action_param1,action_param2,target_type FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT 'line',entry,id,idx,textId,unk2 FROM conversation_data ORDER BY entry,idx;
SELECT 'actor',entry,id,creatureId,duration FROM conversation_creature ORDER BY entry,id;
SELECT 'condition',COUNT(*) FROM conditions;
'@
$sql+=@"
-- Reconstruct the original native conversation even after the live database
-- has been migrated, so this remains a repair test and not just a repeat test.
INSERT IGNORE INTO conversation_data (entry,id,idx,textId,unk1,unk2)
SELECT 2055,id,idx+2,textId+14150,unk1,unk2 FROM conversation_data WHERE entry=9002055;
DELETE FROM conversation_data WHERE entry=9002055;
DELETE FROM conversation_creature WHERE entry=9002055;
UPDATE conversation_creature SET duration=7200,creatureId=IF(id=2,0,creatureId) WHERE entry=2055;
UPDATE smart_scripts SET event_type=60,event_param1=0,event_param2=0 WHERE entryorguid=105016 AND source_type=0 AND id=2;
INSERT IGNORE INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,NegativeCondition,Comment)
VALUES (22,3,105016,29,0,106352,30,1,'Native missing tank'),(22,3,105016,29,0,106354,30,1,'Native missing crushers');
$migration
SELECT 'FIRST';
$snapshot
$migration
SELECT 'SECOND';
$snapshot
SELECT 'END';
SELECT 'binding',COUNT(*) FROM creature_template WHERE entry=105016 AND ScriptName='npc_titanstrike_xr947';
SELECT 'last-child',COUNT(*) FROM conversation_data WHERE entry=9002055 AND id=4364 AND textId=44600 AND idx=4;
SELECT 'hologram-actor',COUNT(*) FROM conversation_creature WHERE entry=9002055 AND id=2 AND creatureId=106376 AND duration=50000;
SELECT 'initial-duration',SUM(duration) FROM conversation_creature WHERE entry=2055;
UPDATE creature_template SET ScriptName='custom' WHERE entry=105016;
UPDATE smart_scripts SET event_type=60,event_param1=0,event_param2=0 WHERE entryorguid=105016 AND source_type=0 AND id=2;
$migration
SELECT 'custom',COUNT(*) FROM creature_template WHERE entry=105016 AND ScriptName='custom';
SELECT 'custom-sai',COUNT(*) FROM smart_scripts WHERE entryorguid=105016 AND source_type=0 AND id=2 AND event_type=60;
"@
$config=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$config[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($config[0]) --port=$($config[1]) --user=$($config[2]) --database=$($config[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Workshop real-schema fixture failed'}}finally{$env:MYSQL_PWD=$previous}
$a=[Array]::IndexOf($rows,'FIRST');$b=[Array]::IndexOf($rows,'SECOND');$d=[Array]::IndexOf($rows,'END')
if(($rows[($a+1)..($b-1)] -join '|') -ne ($rows[($b+1)..($d-1)] -join '|')){throw 'Workshop migration is not idempotent'}
foreach($expected in @("binding`t1","last-child`t1","hologram-actor`t1","initial-duration`t14150","custom`t1","custom-sai`t1","condition`t0","sai`t105016`t0`t2`t4`t38`t2`t2`t53`t0`t105016`t1","sai`t105016`t0`t6`t0`t61`t0`t0`t45`t3`t3`t1")){if($rows -notcontains $expected){throw "Missing workshop result $expected"}}
# Essential native handoff remains independent of conversations: Grif boots XR,
# native SpellHit enables each attacker, XR reaches the console, Mimiron opens
# the weapon, and only the subsequent native 49889 event advances Stage4.
foreach($pattern in @('sai\t10490400\t9\t1\t0\t0\t20000\t20000\t45\t1\t1', 'sai\t106352\t0\t0\t2\t8\t210845', 'sai\t106354\t0\t0\t2\t8\t210845', 'sai\t106428\t9\t1\t0\t0\t0\t0\t205\t92\t49889')){if(!($rows -match $pattern)){throw "Missing native continuation $pattern"}}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TitanstrikeStormCatalog.cpp -ProbeName TitanstrikeStormCatalog -OutputFile "$out/native.tsv"
$native=Get-Content "$out/native.tsv"
foreach($expected in @("EFFECT`t210845`t0`t3`t0`t0`t0`t0`t22`t7`t0`t27","RADIUS`t27`t50`t50","VISUAL`t210836`t55446","VISUAL`t201239`t41416","EFFECT`t210848`t0`t219`t0`t2055`t0`t0`t18`t0`t0`t0")){if($native -notcontains $expected){throw "Native workshop data mismatch $expected"}}
if($VerifyReleaseBinary){
 $oldPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$oldPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_titanstrike_workshop_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Deployed workshop verification failed'}}finally{$env:PYTHONPATH=$oldPython}
}
'PASS: production XR9-47 actor, all kill orders and retries; real-schema native continuation/dialogue and matching 7.3.5 data. Full client Stage4 remains pending.'
