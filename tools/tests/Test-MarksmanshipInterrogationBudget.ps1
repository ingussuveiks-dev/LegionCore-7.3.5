param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-interrogation-budget"
$common="$repo/build-extractors/runtime-test/marksmanship-farewell"
New-Item -ItemType Directory -Force $out | Out-Null
& "$PSScriptRoot/Test-MarksmanshipFarewell.ps1" *> "$out/farewell-prerequisite.log"
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_09_marksmanship_interrogation_lifetimes.sql" -Raw
$reset='DELETE FROM conversation_actor; INSERT INTO conversation_actor SELECT * FROM baseline_actors; DELETE FROM conversation_data; INSERT INTO conversation_data SELECT * FROM saved_lines; DELETE FROM conversation_creature; INSERT INTO conversation_creature SELECT * FROM saved_creatures;'
$sql=@"
CREATE TEMPORARY TABLE saved_actors LIKE conversation_actor;
INSERT INTO saved_actors SELECT * FROM conversation_actor;
CREATE TEMPORARY TABLE conversation_actor LIKE saved_actors;
INSERT INTO conversation_actor SELECT * FROM saved_actors;
UPDATE conversation_actor SET duration=9000 WHERE entry=1033 AND id=0;
UPDATE conversation_actor SET duration=3450 WHERE entry=1556 AND id=0;
CREATE TEMPORARY TABLE baseline_actors LIKE conversation_actor;
INSERT INTO baseline_actors SELECT * FROM conversation_actor;
CREATE TEMPORARY TABLE saved_lines LIKE conversation_data;
INSERT INTO saved_lines SELECT * FROM conversation_data;
CREATE TEMPORARY TABLE conversation_data LIKE saved_lines;
INSERT INTO conversation_data SELECT * FROM saved_lines;
CREATE TEMPORARY TABLE saved_creatures LIKE conversation_creature;
INSERT INTO saved_creatures SELECT * FROM conversation_creature;
CREATE TEMPORARY TABLE conversation_creature LIKE saved_creatures;
INSERT INTO conversation_creature SELECT * FROM saved_creatures;
UPDATE conversation_actor SET duration=27000 WHERE entry=1033 AND id=0;
UPDATE conversation_actor SET duration=10000 WHERE entry=1556 AND id=0;
SELECT 'before-actors',conversation_actor.* FROM conversation_actor ORDER BY entry,id;
SELECT 'before-lines',conversation_data.* FROM conversation_data ORDER BY entry,idx,id;
SELECT 'before-creatures',conversation_creature.* FROM conversation_creature ORDER BY entry,id;
$reset
$migration
$migration
SELECT 'after-actors',conversation_actor.* FROM conversation_actor ORDER BY entry,id;
SELECT 'after-lines',conversation_data.* FROM conversation_data ORDER BY entry,idx,id;
SELECT 'after-creatures',conversation_creature.* FROM conversation_creature ORDER BY entry,id;
SELECT 'duration',entry,SUM(duration)+5000 FROM conversation_actor WHERE entry IN(1033,1556) GROUP BY entry ORDER BY entry;
SELECT 'baseline-actor',entry,id,actorId,creatureId,displayId,unk1,unk2,unk3,duration FROM baseline_actors WHERE entry IN(1033,1556) ORDER BY entry,id;
SELECT 'baseline-line',entry,id,idx,textId,unk1,unk2 FROM saved_lines WHERE entry IN(1033,1556) ORDER BY entry,idx;
SELECT 'actor',entry,id,actorId,creatureId,displayId,unk1,unk2,unk3,duration FROM conversation_actor WHERE entry IN(1033,1556) ORDER BY entry,id;
SELECT 'line',entry,id,idx,textId,unk1,unk2 FROM conversation_data WHERE entry IN(1033,1556) ORDER BY entry,idx;
"@
$captureVariants=@{
 'secondary-duration'='UPDATE conversation_actor SET duration=8000 WHERE entry=1033 AND id=1;'
 'display'='UPDATE conversation_actor SET displayId=28222 WHERE entry=1033 AND id=0;'
 'actor'='UPDATE conversation_actor SET actorId=50832 WHERE entry=1033 AND id=0;'
 'creature'='UPDATE conversation_actor SET creatureId=100836 WHERE entry=1033 AND id=0;'
 'actor-flags'='UPDATE conversation_actor SET unk2=0 WHERE entry=1033 AND id=0;'
 'offset'='UPDATE conversation_data SET textId=9500 WHERE entry=1033 AND id=2543;'
 'speaker'='UPDATE conversation_data SET unk2=0 WHERE entry=1033 AND id=2541;'
 'line'='UPDATE conversation_data SET id=2540 WHERE entry=1033 AND id=2539;'
 'extra-actor'='INSERT INTO conversation_actor SELECT entry,2,actorId,creatureId,displayId,unk1,unk2,unk3,duration FROM baseline_actors WHERE entry=1033 AND id=0;'
 'extra-line'='INSERT INTO conversation_data SELECT entry,999999,3,18000,unk1,unk2 FROM saved_lines WHERE entry=1033 AND id=2539;'
 'mixed-creature'='INSERT INTO conversation_creature(entry,id,creatureId,creatureGuid,unk1,unk2,duration) VALUES(1033,0,100397,0,0,0,1000);'
}
foreach($name in $captureVariants.Keys){$sql+="$reset`n$($captureVariants[$name])`n$migration`nSELECT 'capture-$name',duration FROM conversation_actor WHERE entry=1033 AND id=0;`nSELECT 'independent-$name',duration FROM conversation_actor WHERE entry=1556 AND id=0;`n"}
$introVariants=@{
 'display'='UPDATE conversation_actor SET displayId=28222 WHERE entry=1556 AND id=0;'
 'actor-flags'='UPDATE conversation_actor SET unk1=1 WHERE entry=1556 AND id=0;'
 'offset'='UPDATE conversation_data SET textId=1000 WHERE entry=1556;'
 'speaker'='UPDATE conversation_data SET unk2=1 WHERE entry=1556;'
 'line'='UPDATE conversation_data SET id=3421 WHERE entry=1556;'
 'mixed-creature'='INSERT INTO conversation_creature(entry,id,creatureId,creatureGuid,unk1,unk2,duration) VALUES(1556,0,101269,0,0,0,1000);'
}
foreach($name in $introVariants.Keys){$sql+="$reset`n$($introVariants[$name])`n$migration`nSELECT 'intro-$name',duration FROM conversation_actor WHERE entry=1556 AND id=0;`nSELECT 'independent-intro-$name',duration FROM conversation_actor WHERE entry=1033 AND id=0;`n"}
foreach($entry in @(1033,1556)){$sql+="$reset`nUPDATE conversation_actor SET duration=12345 WHERE entry=$entry AND id=0;`n$migration`nSELECT 'custom-duration-$entry',duration FROM conversation_actor WHERE entry=$entry AND id=0;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Interrogation temporary SQL fixture failed'}}finally{$env:MYSQL_PWD=$previous}
$expected=@("duration`t1033`t41000","duration`t1556`t15000","custom-duration-1033`t12345","custom-duration-1556`t12345")
$expected+=@($captureVariants.Keys | ForEach-Object {"capture-$_`t9000";"independent-$_`t10000"})
$expected+=@($introVariants.Keys | ForEach-Object {"intro-$_`t3450";"independent-intro-$_`t27000"})
foreach($value in $expected){if($rows -notcontains $value){throw "Interrogation mismatch: $value"}}
foreach($kind in @('actors','lines','creatures')){
 $before=@($rows | Where-Object {$_ -like "before-$kind`t*"} | ForEach-Object {$_.Substring(8+$kind.Length)})
 $after=@($rows | Where-Object {$_ -like "after-$kind`t*"} | ForEach-Object {$_.Substring(7+$kind.Length)})
 if(Compare-Object $before $after){throw "Unrelated conversation $kind fields changed"}
}
$definitions=@($rows | Where-Object {$_ -like "actor`t*" -or $_ -like "line`t*"})
[IO.File]::WriteAllLines("$out/conversations.tsv",[string[]]$definitions)
foreach($entry in @(1033,1556)){[IO.File]::WriteAllLines("$out/conversation-$entry.tsv",[string[]]@($definitions | Where-Object {$_.Split("`t")[1] -eq [string]$entry}))}
$baseline=@($rows | Where-Object {$_ -like "baseline-actor`t*" -or $_ -like "baseline-line`t*"} | ForEach-Object {$_.Substring(9)})
foreach($entry in @(1033,1556)){[IO.File]::WriteAllLines("$out/baseline-conversation-$entry.tsv",[string[]]@($baseline | Where-Object {$_.Split("`t")[1] -eq [string]$entry}))}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$common`" `"$PSScriptRoot/ConversationInterrogationTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/conversations.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native actor conversation lifetime regression failed'}
if($VerifyReleaseBinary){
 $previousPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_conversation_farewell_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical conversation lifetime PE verification failed'}}finally{$env:PYTHONPATH=$previousPython}
}
'PASS: two isolated native actor-duration updates preserve every other actor/line/creature field, refuse custom definitions, apply twice and retain independent repair of the unmodified conversation'
