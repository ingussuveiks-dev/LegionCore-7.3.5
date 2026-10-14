$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/ritual-reagents-loot"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_14_09_ritual_reagents_loot_links.sql" -Raw
# Temporary tables shadow real tables for this connection only. Start with the
# observed broken state even after the migration has been installed in the DB.
$fixture=@"
CREATE TEMPORARY TABLE saved_gameobject_template LIKE gameobject_template;
INSERT INTO saved_gameobject_template SELECT * FROM gameobject_template WHERE entry IN (249742,249744,249748,245924);
CREATE TEMPORARY TABLE gameobject_template LIKE saved_gameobject_template;
INSERT INTO gameobject_template SELECT * FROM saved_gameobject_template;
UPDATE gameobject_template SET Data1=0 WHERE entry IN (249742,249744,249748);
SELECT 'before',COUNT(*) FROM gameobject_template WHERE entry IN (249742,249744,249748) AND Data1=0;
$migration
$migration
SELECT g.entry,g.type,g.Data1,l.Item,l.Chance,l.QuestRequired,l.MinCount,l.MaxCount,o.QuestID,o.Amount
FROM gameobject_template g
JOIN gameobject_loot_template l ON l.Entry=g.Data1
JOIN quest_objectives o ON o.ObjectID=l.Item AND o.Type=1 AND o.QuestID=42128
WHERE g.entry IN (249742,249744,249748);
SELECT 'unrelated',COUNT(*) FROM gameobject_template g JOIN saved_gameobject_template s ON s.entry=g.entry WHERE g.entry=245924 AND g.Data1=s.Data1;
SELECT 'preserved',COUNT(*) FROM gameobject_template g JOIN saved_gameobject_template s ON s.entry=g.entry WHERE g.entry IN (249742,249744,249748) AND g.type=s.type AND g.flags=s.flags AND g.Data0=s.Data0 AND g.Data8=s.Data8 AND g.Data30=s.Data30 AND g.questItem1=s.questItem1;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $rows=@($fixture | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Ritual reagent SQL fixture failed'}
} finally {$env:MYSQL_PWD=$old}
if($rows -notcontains "before`t3" -or $rows -notcontains "unrelated`t1" -or $rows -notcontains "preserved`t3"){throw 'Missing baseline/isolation/preservation check'}
$loot=@($rows | Where-Object {$_ -match '^2497\d+\t'})
if($loot.Count -ne 3){throw 'Expected exactly three objective-linked loot rows'}
$expected=@{249742=136983;249744=136985;249748=136987}
foreach($line in $loot){
 $v=$line.Split("`t")
 if($v.Count -ne 10 -or $v[1] -ne '3' -or $v[0] -ne $v[2] -or [int]$v[3] -ne $expected[[int]$v[0]] -or [double]$v[4] -ne 100 -or $v[5] -ne '1' -or $v[6] -ne '1' -or $v[7] -ne '1' -or $v[8] -ne '42128' -or $v[9] -ne '1'){throw "Incorrect quest loot mapping: $line"}
}
[IO.File]::WriteAllLines("$output/loot.tsv",[string[]]$loot)
$source=Get-Content "$repo/src/server/game/Entities/GameObject/GameObjectData.cpp" -Raw
$a=$source.IndexOf('uint32 GameObjectTemplate::GetLootId() const')
$b=$source.IndexOf('uint32 GameObjectTemplate::GetGossipMenuId() const',$a)
if($a -lt 0 -or $b -lt $a){throw 'Native loot selector missing'}
[IO.File]::WriteAllText("$output/GetLootId.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$output`" `"$PSScriptRoot/RitualReagentsLootTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$output/test.exe`" `"$output/loot.tsv`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Native reagent loot selection regression failed'}
