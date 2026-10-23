param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-farewell"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_02_marksmanship_farewell_lifetime.sql" -Raw
$reset="DELETE FROM conversation_actor; UPDATE conversation_creature SET duration=2650 WHERE entry=1072;"
$sql=@"
CREATE TEMPORARY TABLE saved_creatures LIKE conversation_creature;
INSERT INTO saved_creatures SELECT * FROM conversation_creature;
CREATE TEMPORARY TABLE conversation_creature LIKE saved_creatures;
INSERT INTO conversation_creature SELECT * FROM saved_creatures;
CREATE TEMPORARY TABLE saved_lines LIKE conversation_data;
INSERT INTO saved_lines SELECT * FROM conversation_data;
CREATE TEMPORARY TABLE conversation_data LIKE saved_lines;
INSERT INTO conversation_data SELECT * FROM saved_lines;
CREATE TEMPORARY TABLE saved_actors LIKE conversation_actor;
CREATE TEMPORARY TABLE conversation_actor LIKE saved_actors;
$reset
UPDATE conversation_creature SET duration=25000 WHERE entry=1072 AND id=0;
SELECT 'before',conversation_creature.* FROM conversation_creature ORDER BY entry,id;
$reset
$migration
$migration
SELECT 'after',conversation_creature.* FROM conversation_creature ORDER BY entry,id;
SELECT 'sum',SUM(duration)+5000 FROM conversation_creature WHERE entry=1072;
SELECT 'creature',entry,id,creatureId,creatureGuid,unk1,unk2,duration FROM conversation_creature WHERE entry=1072 ORDER BY id;
SELECT 'line',entry,id,idx,textId,unk1,unk2 FROM conversation_data WHERE entry=1072 ORDER BY idx;
SELECT 'changed-lines',COUNT(*) FROM conversation_data current JOIN saved_lines old ON current.entry=old.entry AND current.id=old.id WHERE current.idx<>old.idx OR current.textId<>old.textId OR current.unk1<>old.unk1 OR current.unk2<>old.unk2;
$reset
UPDATE conversation_creature SET duration=3000 WHERE entry=1072 AND id=1;
$migration
SELECT 'custom-duration',duration FROM conversation_creature WHERE entry=1072 AND id=0;
$reset
UPDATE conversation_creature SET creatureId=100836 WHERE entry=1072 AND id=1;
$migration
SELECT 'custom-creature',duration FROM conversation_creature WHERE entry=1072 AND id=0;
UPDATE conversation_creature SET creatureId=100398 WHERE entry=1072 AND id=1;
$reset
UPDATE conversation_data SET textId=23000 WHERE entry=1072 AND id=2631;
$migration
SELECT 'custom-line',duration FROM conversation_creature WHERE entry=1072 AND id=0;
UPDATE conversation_data SET textId=23050 WHERE entry=1072 AND id=2631;
$reset
INSERT INTO conversation_actor(entry,id,actorId,creatureId,displayId,unk1,unk2,unk3,duration) VALUES(1072,0,1,1,1,0,0,0,1);
$migration
SELECT 'custom-actor',duration FROM conversation_creature WHERE entry=1072 AND id=0;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Farewell temporary SQL fixture failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("sum`t35300","changed-lines`t0","custom-duration`t2650","custom-creature`t2650","custom-line`t2650","custom-actor`t2650")){if($rows -notcontains $expected){throw "Farewell mismatch: $expected"}}
$before=@($rows | Where-Object {$_ -like "before`t*"} | ForEach-Object {$_.Substring(7)})
$after=@($rows | Where-Object {$_ -like "after`t*"} | ForEach-Object {$_.Substring(6)})
if(Compare-Object $before $after){throw 'Unrelated conversation creature fields changed'}
[IO.File]::WriteAllLines("$out/conversation.tsv",[string[]]@($rows | Where-Object {$_ -like "creature`t*" -or $_ -like "line`t*"}))
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Globals/ConversationData.h" -Raw
[IO.File]::WriteAllText("$out/ConversationStructs.inc",(Section $source 'struct ConversationActor' 'typedef std::unordered_map'))
$source=Get-Content "$repo/src/server/game/Entities/Conversation/Conversation.cpp" -Raw
[IO.File]::WriteAllText("$out/ConversationData.inc",$source.Substring($source.IndexOf('bool Conversation::GenerateDynamicFieldData(')))
[IO.File]::WriteAllText("$out/Lifetime.inc",(Section $source '    uint32 duration = 0;' '    if (!GetMap()->AddToMap(this))'))
[IO.File]::WriteAllText("$out/Update.inc",(Section $source 'void Conversation::Update(uint32 p_time)' 'void Conversation::Remove()'))
[IO.File]::WriteAllText("$out/Playback.inc",(Section $source 'bool Conversation::CanNeverSee2(' 'bool Conversation::LoadFromDB('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/ConversationFarewellTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/conversation.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native farewell regression failed'}
if($VerifyReleaseBinary){$previousPython=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_conversation_farewell_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical farewell PE verification failed'}}finally{$env:PYTHONPATH=$previousPython}}
'PASS: migration changes only native1072 slot0 lifetime, twice-idempotent, preserving all other conversations/lines and custom definitions'
