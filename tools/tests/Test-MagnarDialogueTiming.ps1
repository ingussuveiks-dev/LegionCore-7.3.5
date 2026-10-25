param([switch]$BaselineSql,[switch]$VerifyReleaseBinary,[string]$ClientPath='D:/wow/Legion7.3.5')
$ErrorActionPreference='Stop'
if($BaselineSql -and $VerifyReleaseBinary){throw 'A baseline cannot verify installed timing'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-MagnarConversations.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary -ClientPath $ClientPath
if(!$? -or $LASTEXITCODE){throw 'Native conversation/audio preparation failed'}
$common="$repo/build-extractors/runtime-test/hruthnir-conversation"
$out="$repo/build-extractors/runtime-test/magnar-dialogue-timing"
New-Item -ItemType Directory -Force $out|Out-Null
$audio=Get-Content "$repo/build-extractors/runtime-test/magnar-conversations/audio-summary.json" -Raw|ConvertFrom-Json
$starts=@{}
$voiceRows=@(foreach($budget in $audio.Budgets){$start=0;foreach($line in $budget.Lines){$starts[[uint32]$line.Line]=$start;"$($line.Line)`t$($line.VoiceMs)`t$($line.AdditionalMs)";$start+=[uint32]$line.VoiceMs+[uint32]$line.AdditionalMs}})
foreach($expected in @(@(1335,0),@(1336,5646),@(1337,19930),@(1341,0),@(1342,8618),@(1343,23424),@(1344,34064))){if($starts[[uint32]$expected[0]] -ne $expected[1]){throw 'Fresh complete voice timing differs; re-audit migration'}}
[IO.File]::WriteAllLines("$out/voices.tsv",[string[]]$voiceRows)
$repair=Get-Content "$repo/sql/updates/world/2026_10_25_05_magnar_dialogue_start_times.sql" -Raw
$migration=if($BaselineSql){''}else{$repair}
$reset=@'
DELETE FROM conversation_creature; INSERT INTO conversation_creature SELECT * FROM saved_creatures;
DELETE FROM conversation_data; INSERT INTO conversation_data SELECT * FROM saved_lines;
DELETE FROM conversation_actor; INSERT INTO conversation_actor SELECT * FROM saved_actors;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_creatures LIKE conversation_creature;
INSERT INTO saved_creatures SELECT * FROM conversation_creature;
CREATE TEMPORARY TABLE conversation_creature LIKE saved_creatures;
CREATE TEMPORARY TABLE saved_lines LIKE conversation_data;
INSERT INTO saved_lines SELECT * FROM conversation_data;
UPDATE saved_lines SET textId=CASE id WHEN 1336 THEN 2350 WHEN 1337 THEN 7300 ELSE textId END WHERE entry=498;
UPDATE saved_lines SET textId=CASE id WHEN 1342 THEN 3950 WHEN 1343 THEN 12900 WHEN 1344 THEN 21350 ELSE textId END WHERE entry=504;
CREATE TEMPORARY TABLE conversation_data LIKE saved_lines;
CREATE TEMPORARY TABLE saved_actors LIKE conversation_actor;
INSERT INTO saved_actors SELECT * FROM conversation_actor;
CREATE TEMPORARY TABLE conversation_actor LIKE saved_actors;
$reset
$migration
SELECT 'FIRST',conversation_data.* FROM conversation_data ORDER BY entry,idx;
$migration
SELECT 'SECOND',conversation_data.* FROM conversation_data ORDER BY entry,idx;
SELECT 'line',entry,id,idx,textId,unk1,unk2 FROM conversation_data WHERE entry IN(498,504) ORDER BY entry,idx;
SELECT 'creature',entry,id,creatureId,creatureGuid,unk1,unk2,duration FROM conversation_creature WHERE entry IN(498,504) ORDER BY entry,id;
SELECT 'preserved-lines',COUNT(*) FROM conversation_data current JOIN saved_lines old USING(entry,id)
WHERE current.idx<>old.idx OR current.unk1<>old.unk1 OR current.unk2<>old.unk2 OR (current.entry NOT IN(498,504) AND current.textId<>old.textId);
SELECT 'preserved-actors',COUNT(*) FROM conversation_creature current JOIN saved_creatures old USING(entry,id)
WHERE current.creatureId<>old.creatureId OR current.creatureGuid<>old.creatureGuid OR current.unk1<>old.unk1 OR current.unk2<>old.unk2 OR current.duration<>old.duration;
SELECT 'model-count',(SELECT COUNT(*) FROM conversation_actor)=(SELECT COUNT(*) FROM saved_actors);
$reset
UPDATE conversation_data SET textId=5646 WHERE entry=498 AND id=1336;
UPDATE conversation_data SET textId=23424 WHERE entry=504 AND id=1343;
$repair
SELECT 'mixed498',textId FROM conversation_data WHERE entry=498 AND idx=2;
SELECT 'mixed504',textId FROM conversation_data WHERE entry=504 AND idx=3;
"@
$variants=[ordered]@{
 duration='UPDATE conversation_creature SET duration=1 WHERE entry={0} AND id=0;'
 actor='UPDATE conversation_creature SET creatureId=1 WHERE entry={0} AND id=0;'
 guid='UPDATE conversation_creature SET creatureGuid=1 WHERE entry={0} AND id=1;'
 actor_flags='UPDATE conversation_creature SET unk1=0 WHERE entry={0} AND id=0;'
 actor_flags2='UPDATE conversation_creature SET unk2=0 WHERE entry={0} AND id=1;'
 start='UPDATE conversation_data SET textId=1 WHERE entry={0} AND idx=1;'
 first_start='UPDATE conversation_data SET textId=1 WHERE entry={0} AND idx=0;'
 index='UPDATE conversation_data SET idx=8 WHERE entry={0} AND idx=1;'
 camera='UPDATE conversation_data SET unk1=1 WHERE entry={0} AND idx=1;'
 packed='UPDATE conversation_data SET unk2=256 WHERE entry={0} AND idx=2;'
 missing_actor='DELETE FROM conversation_creature WHERE entry={0} AND id=1;'
 missing_line='DELETE FROM conversation_data WHERE entry={0} AND idx=2;'
 extra_actor='INSERT INTO conversation_creature(entry,id) VALUES({0},3);'
 extra_line='INSERT INTO conversation_data(entry,id,idx) VALUES({0},999999,10);'
 model='INSERT INTO conversation_actor(entry,id) VALUES({0},0);'
}
foreach($identity in @(498,504)){foreach($name in $variants.Keys){$other=if($identity -eq498){504}else{498};$sql+="`n$reset`n$($variants[$name] -f $identity)`n$repair`nSELECT '$identity-$name',textId FROM conversation_data WHERE entry=$identity AND idx=0; SELECT '$identity-$name-other',textId FROM conversation_data WHERE entry=$other AND idx=1; SELECT '$identity-$name-probe',textId FROM conversation_data WHERE entry=$identity AND id=IF($identity=498,1336,1342);"}}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'TEMP dialogue timing fixtures failed'}
 if($VerifyReleaseBinary){$live=@('SELECT entry,id,idx,textId,unk1,unk2 FROM conversation_data WHERE entry IN(498,504) ORDER BY entry,idx;'|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Live timing read failed'}}
}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("preserved-lines`t0","preserved-actors`t0","model-count`t1","mixed498`t19930","mixed504`t34064")){if($rows -notcontains $expected){throw "Timing preservation differs: $expected"}}
$first=@($rows|Where-Object {$_ -like "FIRST`t*"}|ForEach-Object {$_.Substring(6)});$second=@($rows|Where-Object {$_ -like "SECOND`t*"}|ForEach-Object {$_.Substring(7)})
if(Compare-Object $first $second){throw 'Timing migration not idempotent'}
foreach($identity in @(498,504)){foreach($name in $variants.Keys){$other=if($identity -eq498){8618}else{5646};$probe=if($name -eq 'start'){1}elseif($identity -eq498){2350}else{3950};$firstStart=if($name -eq 'first_start'){1}else{0};if($rows -notcontains "$identity-$name`t$firstStart" -or $rows -notcontains "$identity-$name-other`t$other" -or $rows -notcontains "$identity-$name-probe`t$probe"){throw "Independent custom guard differs: $identity-$name"}}}
$payload=@($rows|Where-Object {$_ -like "line`t*" -or $_ -like "creature`t*"})
if($VerifyReleaseBinary){$expected=@($payload|Where-Object {$_ -like "line`t*"}|ForEach-Object {$_.Substring(5)});if(Compare-Object $expected $live){throw 'Canonical live timing migration not deployed'}}
[IO.File]::WriteAllLines("$out/conversation.tsv",[string[]]$payload)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($BaselineSql){'/DTIMING_BASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$common`" `"$PSScriptRoot/MagnarDialogueTimingTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/conversation.tsv`" `"$out/voices.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native dialogue timing regression failed'}
'PASS: independent timing-only TEMP migration, two mixed retries and30 customized/missing definitions preserve actor/line flags and visibility budgets; actual client playback remains pending'
