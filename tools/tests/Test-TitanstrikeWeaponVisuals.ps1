$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/titanstrike-weapon-visuals"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_23_09_titanstrike_weapon_visuals.sql" -Raw
$snapshot=@'
SELECT 'display',Entry,DisplayId1,DisplayId2,DisplayId3,DisplayId4 FROM creature_template_wdb ORDER BY Entry;
SELECT 'movement',CreatureId,Ground,Flight FROM creature_template_movement ORDER BY CreatureId;
SELECT 'theft',id,event_param1,event_param2,action_type,action_param1,action_param2,target_type,target_param1 FROM smart_scripts ORDER BY entryorguid,source_type,id;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_wdb LIKE creature_template_wdb;
INSERT INTO saved_wdb SELECT * FROM creature_template_wdb WHERE Entry IN(106428,106745);
CREATE TEMPORARY TABLE creature_template_wdb LIKE saved_wdb;
INSERT INTO creature_template_wdb SELECT * FROM saved_wdb;
UPDATE creature_template_wdb SET DisplayId1=42428,DisplayId2=69591;
CREATE TEMPORARY TABLE saved_movement LIKE creature_template_movement;
INSERT INTO saved_movement SELECT * FROM creature_template_movement WHERE CreatureId IN(106428,106745);
CREATE TEMPORARY TABLE creature_template_movement LIKE saved_movement;
INSERT INTO creature_template_movement SELECT * FROM saved_movement;
UPDATE creature_template_movement SET Ground=1,Flight=2;
CREATE TEMPORARY TABLE saved_sai LIKE smart_scripts;
INSERT INTO saved_sai SELECT * FROM smart_scripts WHERE entryorguid=10494902 AND source_type=9;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_sai;
INSERT INTO smart_scripts SELECT * FROM saved_sai;
-- Normalize either pre/post-migration state back to the original ten rows.
DELETE FROM smart_scripts WHERE id=5 AND action_type=40 AND action_param1=2;
UPDATE smart_scripts SET id=id-1 WHERE id>=6 AND (SELECT COUNT(*) FROM saved_sai)=11 ORDER BY id;
$migration
SELECT 'FIRST';
$snapshot
$migration
SELECT 'SECOND';
$snapshot
SELECT 'END';
SELECT 'timers',SUM(event_param1),SUM(event_param2),COUNT(*) FROM smart_scripts;
SELECT 'equip',ItemID5 FROM creature_equip_template WHERE CreatureID=104949 AND ID=1;
-- A customized list/model remains intact.
DELETE FROM smart_scripts;
INSERT INTO smart_scripts SELECT * FROM saved_sai;
UPDATE smart_scripts SET action_param1=123456 WHERE id=0;
UPDATE creature_template_wdb SET DisplayId1=99999,DisplayId2=69591 WHERE Entry=106428;
$migration
SELECT 'custom-list',COUNT(*) FROM smart_scripts WHERE id=0 AND action_param1=123456;
SELECT 'custom-model',DisplayId1,DisplayId2 FROM creature_template_wdb WHERE Entry=106428;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Weapon visual temporary SQL fixture failed'}}finally{$env:MYSQL_PWD=$previous}
$a=[Array]::IndexOf($rows,'FIRST');$b=[Array]::IndexOf($rows,'SECOND');$d=[Array]::IndexOf($rows,'END')
$first=@($rows[($a+1)..($b-1)]);$second=@($rows[($b+1)..($d-1)])
if($first.Count -ne 15 -or ($first -join '|') -ne ($second -join '|')){throw 'Weapon visual migration changed on repeat'}
foreach($expected in @("display`t106428`t69591`t0`t0`t0","display`t106745`t69591`t0`t0`t0","movement`t106428`t0`t1","movement`t106745`t0`t1","theft`t4`t1500`t1500`t45`t1`t1`t19`t106428","theft`t5`t0`t0`t40`t2`t0`t1`t0","theft`t6`t2000`t2000`t53`t0`t10494904`t1`t0")){if($first -notcontains $expected){throw "Weapon lifecycle mismatch $expected"}}
foreach($expected in @("timers`t28500`t28500`t11","equip`t128861","custom-list`t1","custom-model`t99999`t69591")){if($rows -notcontains $expected){throw "Weapon boundary mismatch $expected"}}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TitanstrikeTheftCatalog.cpp -ProbeName TitanstrikeTheftCatalog -OutputFile "$out/native.tsv"
$native=Get-Content "$out/native.tsv"
foreach($expected in @("MODEL`t42428`t24`t124637`t0`t0","MODEL`t69591`t1731`t124640`t67322`t0","KIT_EFFECT`t67322`t143770`t2`t322438","EFFECT_NAME`t27401`t0`t1346250`t0`t1`t0","ATTACH`t67322`t322438`t27401`t1346250`t0`t-1`t0`t1.5`t34`t0","ITEM`t128861`t26")){if($native -notcontains $expected){throw "Native rifle visual mismatch $expected"}}
# Execute the actual SmartAI dispatcher branch and Unit's packed sheath setter.
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$a=$s.IndexOf('        case SMART_ACTION_SET_SHEATH:');$b=$s.IndexOf('        case SMART_ACTION_FORCE_DESPAWN:',$a)
[IO.File]::WriteAllText("$out/SheathAction.inc",$s.Substring($a,$b-$a))
$s=Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw
$setter=[regex]::Match($s,'virtual void SetSheath\(SheathState sheathed\) \{[^}]+\}').Value
if(!$setter){throw 'Native sheath setter missing'}
[IO.File]::WriteAllText("$out/SheathSetter.inc",$setter)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/TitanstrikeWeaponVisualsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native ranged sheath action failed'}
'PASS: native artifact state-kit/attachment/model, deterministic elevated props, ranged draw at pickup before portal path, original timers, repeat/custom SQL boundaries. Rendering and full client theft/pickup remain pending.'
