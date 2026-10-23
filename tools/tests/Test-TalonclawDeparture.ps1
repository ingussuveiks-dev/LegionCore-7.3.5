param([switch]$BaselineSql)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/talonclaw-departure"
New-Item -ItemType Directory -Force $out | Out-Null
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
$start=$source.IndexOf('namespace TalonclawDeparture');$end=$source.IndexOf('void AddSC_artifact_acquisition()',$start)
if($start -lt 0 -or $end -lt $start){throw 'Missing production departure lifecycle'}
[IO.File]::WriteAllText("$out/Departure.inc",$source.Substring($start,$end-$start))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/TalonclawDepartureTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native Talonclaw departure regression failed'}
$migration=if($BaselineSql){''}else{Get-Content "$repo/sql/updates/world/2026_10_23_13_talonclaw_departure.sql" -Raw}
$sql=@"
CREATE TEMPORARY TABLE saved_go LIKE gameobject_template;
INSERT INTO saved_go SELECT * FROM gameobject_template WHERE entry=9000575;
CREATE TEMPORARY TABLE gameobject_template LIKE saved_go;
INSERT INTO gameobject_template SELECT * FROM saved_go;
DELETE FROM gameobject_template WHERE entry=9000575;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid IN(248834,100005) AND source_type IN(0,1);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
DELETE FROM smart_scripts WHERE entryorguid=248834 AND id=3;
UPDATE smart_scripts SET link=3 WHERE entryorguid=248834 AND id=1;
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,event_chance,event_flags,action_type,action_param1,action_param2,target_type,target_x,target_y,target_z,target_o,comment)
VALUES(248834,1,3,61,100,1,12,100005,8,8,3878.35,1636.92,-0.0612079,1.89629,'Original early Dorro summon');
CREATE TEMPORARY TABLE saved_text LIKE creature_text;
INSERT INTO saved_text SELECT * FROM creature_text WHERE CreatureID=100005;
CREATE TEMPORARY TABLE creature_text LIKE saved_text;
INSERT INTO creature_text SELECT * FROM saved_text;
DELETE FROM creature_text WHERE GroupID=1;
$migration
$migration
SELECT 'portal',type,displayId,flags,Data0,Data10,ScriptName FROM gameobject_template WHERE entry=9000575;
SELECT 'speech',COUNT(*) FROM creature_text WHERE CreatureID=100005 AND GroupID=1 AND BroadcastTextID=106397 AND Type=12;
SELECT 'first-speech',COUNT(*) FROM creature_text WHERE CreatureID=100005 AND GroupID=0 AND BroadcastTextID=103406;
SELECT 'native-ai',COUNT(*) FROM smart_scripts WHERE entryorguid=100005 AND source_type=0 AND id=0 AND event_type=60 AND action_type=1 AND action_param1=0 AND event_flags=1;
SELECT 'scene',COUNT(*) FROM smart_scripts WHERE entryorguid=248834 AND source_type=1 AND id=0 AND link=1 AND event_type=70 AND action_type=85 AND action_param1=205589;
SELECT 'criterion',COUNT(*) FROM smart_scripts WHERE entryorguid=248834 AND source_type=1 AND id=1 AND link=0 AND event_type=61 AND action_type=205 AND action_param1=92 AND action_param2=47689;
SELECT 'early-actor',COUNT(*) FROM smart_scripts WHERE entryorguid=248834 AND id=3;
UPDATE gameobject_template SET ScriptName='custom_portal' WHERE entry=9000575;
UPDATE creature_text SET BroadcastTextID=999999 WHERE GroupID=1;
DELETE FROM smart_scripts WHERE entryorguid=248834 AND id=3;
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,event_chance,action_type,action_param1,comment) VALUES(248834,1,3,61,100,12,100005,'Custom preserved chain');
UPDATE smart_scripts SET link=3 WHERE entryorguid=248834 AND id=1;
$migration
SELECT 'custom-portal',COUNT(*) FROM gameobject_template WHERE entry=9000575 AND ScriptName='custom_portal';
SELECT 'custom-text',COUNT(*) FROM creature_text WHERE GroupID=1 AND BroadcastTextID=999999;
SELECT 'custom-chain',COUNT(*) FROM smart_scripts WHERE entryorguid=248834 AND ((id=1 AND link=3) OR (id=3 AND action_param1=100005));
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Departure temporary SQL fixture failed'}
}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("portal`t10`t8111`t0`t0`t0`tgo_talonclaw_return_portal","speech`t1","first-speech`t1","native-ai`t1","scene`t1","criterion`t1","early-actor`t0","custom-portal`t1","custom-text`t1","custom-chain`t2")){
 if($rows -notcontains $expected){throw "Talonclaw departure fixture mismatch: $expected"}
}
'PASS: repeated actual-schema departure migration preserves native pickup/criterion/first speech, replaces only early summon, adds native second speech and isolated portal, preserves custom definitions'
