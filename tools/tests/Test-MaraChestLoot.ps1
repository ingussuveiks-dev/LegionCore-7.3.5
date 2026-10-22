$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/mara-chest-loot"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_22_03_mara_battered_chest_loot.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_gameobject_template LIKE gameobject_template;
INSERT INTO saved_gameobject_template SELECT * FROM gameobject_template WHERE entry IN(245479,2843);
CREATE TEMPORARY TABLE gameobject_template LIKE saved_gameobject_template;
INSERT INTO gameobject_template SELECT * FROM saved_gameobject_template;
UPDATE gameobject_template SET Data1=0 WHERE entry=245479;
SELECT 'broken',Data1 FROM gameobject_template WHERE entry=245479;
$migration
$migration
SELECT 'link',g.entry,g.type,g.Data1,COUNT(l.Entry),SUM(l.Currency=1220 AND l.MinCount=70 AND l.MaxCount=70),SUM(l.Item IN(131781,138839))
FROM gameobject_template g JOIN gameobject_loot_template l ON l.Entry=g.Data1 WHERE g.entry=245479 GROUP BY g.entry;
SELECT 'encounter',COUNT(*) FROM smart_scripts WHERE entryorguid=100238 AND source_type=0 AND id=3 AND action_type=50 AND action_param1=245479;
SELECT 'unrelated',COUNT(*) FROM gameobject_template g JOIN saved_gameobject_template s ON s.entry=g.entry WHERE g.entry=2843 AND g.Data1=s.Data1;
UPDATE gameobject_template SET Data1=(SELECT Data1 FROM saved_gameobject_template WHERE entry=245479) WHERE entry=245479;
SELECT 'preserved',COUNT(*) FROM gameobject_template g JOIN saved_gameobject_template s ON g.entry=s.entry WHERE g.entry=245479 AND MD5(CONCAT_WS('|',g.type,g.flags,g.Data0,g.Data1,g.Data2,g.Data3,g.Data8,g.Data15,g.Data17,g.Data27,g.Data30,g.AIName,g.ScriptName))=MD5(CONCAT_WS('|',s.type,s.flags,s.Data0,s.Data1,s.Data2,s.Data3,s.Data8,s.Data15,s.Data17,s.Data27,s.Data30,s.AIName,s.ScriptName));
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Mara chest SQL replay failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @("broken`t0","link`t245479`t3`t245479`t3`t1`t2","encounter`t1","unrelated`t1","preserved`t1")){if($rows -notcontains $line){throw "Mara chest mismatch: $line"}}
$source=Get-Content "$repo/src/server/game/Entities/GameObject/GameObjectData.cpp" -Raw
$a=$source.IndexOf('uint32 GameObjectTemplate::GetLootId() const');$b=$source.IndexOf('uint32 GameObjectTemplate::GetGossipMenuId() const',$a)
if($a -lt 0 -or $b -lt $a){throw 'Missing native loot selector'}
[IO.File]::WriteAllText("$out/GetLootId.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/MaraChestLootTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif %errorlevel% neq 0 exit /b %errorlevel%`n`"$out/test.exe`"`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Mara native loot regression failed'}
'PASS: missing Mara reward link repaired twice; existing currency/items, encounter summon, personal flags and unrelated chest preserved.'
