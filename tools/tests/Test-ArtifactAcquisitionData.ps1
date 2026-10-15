param([string]$MigrationFile="$PSScriptRoot/../../sql/updates/world/2026_10_15_25_artifact_acquisition_loot.sql")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-acquisition"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content $MigrationFile -Raw
$ids='249812,248572,246699,249814,251049,246021,249821,246437'
$sql=@"
CREATE TEMPORARY TABLE saved_go LIKE gameobject_template;
INSERT INTO saved_go SELECT * FROM gameobject_template WHERE entry IN($ids,252054);
CREATE TEMPORARY TABLE gameobject_template LIKE saved_go;
INSERT INTO gameobject_template SELECT * FROM saved_go;
CREATE TEMPORARY TABLE saved_sai LIKE smart_scripts;
INSERT INTO saved_sai SELECT * FROM smart_scripts WHERE source_type=1 AND entryorguid IN($ids,252054);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_sai;
INSERT INTO smart_scripts SELECT * FROM saved_sai;
UPDATE gameobject_template SET Data1=0,ScriptName='' WHERE entry IN($ids);
$migration
$migration
SELECT 'loot',g.entry,g.type,g.Data1,l.Item,l.Chance,l.MinCount,l.MaxCount,g.ScriptName
FROM gameobject_template g JOIN gameobject_loot_template l ON l.Entry=g.Data1 WHERE g.entry IN($ids);
SELECT 'settings',COUNT(*) FROM gameobject_template g JOIN saved_go s ON s.entry=g.entry WHERE g.entry IN($ids) AND g.flags=s.flags AND g.Data0=s.Data0 AND g.Data8=s.Data8 AND g.Data30=s.Data30 AND g.AIName=s.AIName AND g.questItem1=s.questItem1;
SELECT 'unrelated',COUNT(*) FROM gameobject_template g JOIN saved_go s ON s.entry=g.entry WHERE g.entry=252054 AND g.Data1=s.Data1 AND g.ScriptName=s.ScriptName;
SELECT 'root',COUNT(DISTINCT entryorguid) FROM smart_scripts WHERE entryorguid IN($ids) AND source_type=1 AND event_type=70 AND event_param1=3;
SELECT 'gossip',COUNT(*) FROM smart_scripts WHERE source_type=1 AND event_type=64 AND action_type=85 AND ((entryorguid=249812 AND id=2 AND action_param1=212010) OR (entryorguid=248572 AND id=2 AND action_param1=208011) OR (entryorguid=246437 AND id=1 AND action_param1=205387));
SELECT 'broken_link',COUNT(*) FROM smart_scripts s LEFT JOIN smart_scripts t ON t.entryorguid=s.entryorguid AND t.source_type=s.source_type AND t.id=s.link AND t.event_type=61 WHERE s.link<>0 AND s.entryorguid IN($ids) AND s.source_type=1 AND t.id IS NULL;
UPDATE gameobject_template SET ScriptName='custom_script',Data1=987654 WHERE entry=246021;
$migration
SELECT 'custom',COUNT(*) FROM gameobject_template WHERE entry=246021 AND ScriptName='custom_script' AND Data1=987654;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Artifact loot SQL fixture failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @("settings`t8","unrelated`t1","root`t8","gossip`t0","broken_link`t0","custom`t1")){if($rows -notcontains $line){throw "Artifact fixture mismatch: $line"}}
$expected=@{249812=128861;248572=128868;246699=128403;249814=127857;251049=128862;246021=128942;249821=128943;246437=128858}
$loot=@($rows | Where-Object {$_ -like "loot`t*"})
if($loot.Count -ne 8){throw 'Expected exactly eight artifact loot rows'}
foreach($line in $loot){$v=$line.Split("`t");if($v.Count -ne 9 -or $v[2] -ne '3' -or $v[1] -ne $v[3] -or [int]$v[4] -ne $expected[[int]$v[1]] -or [double]$v[5] -ne 100 -or $v[6] -ne '1' -or $v[7] -ne '1' -or $v[8] -ne 'go_artifact_acquisition'){throw "Bad artifact loot mapping: $line"}}
[IO.File]::WriteAllLines("$out/loot.tsv",[string[]]@($loot | ForEach-Object {$_.Substring(5)}))
$source=Get-Content "$repo/src/server/game/Entities/GameObject/GameObjectData.cpp" -Raw
$a=$source.IndexOf('uint32 GameObjectTemplate::GetLootId() const');$b=$source.IndexOf('uint32 GameObjectTemplate::GetGossipMenuId() const',$a)
[IO.File]::WriteAllText("$out/GetLootId.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/ArtifactLootDataTest.cpp`" /Fe`"$out/data-test.exe`" /Fo`"$out/data-test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/data-test.exe`" `"$out/loot.tsv`"`r`n"
[IO.File]::WriteAllText("$out/data-run.cmd",$cmd);& "$out/data-run.cmd"
if($LASTEXITCODE){throw 'Native artifact loot selector regression failed'}
'PASS: eight loot links, repeat migration, native completed-loot roots and links, removed early credit, preserved settings/custom handlers'
