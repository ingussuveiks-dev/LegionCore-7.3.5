param(
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$Migration
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$runtime = "$repo/build-extractors/bin/Release"
$out = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $out | Out-Null
if (!$Migration) {
    $Migration = "$repo/sql/updates/world/2026_11_25_04_dalaran_kula_patrol.sql"
}
$migrationText = Get-Content -LiteralPath $Migration -Raw

# Original path 272369 from the bundled build 26972 world data.
$original = @(
    '-824.491 4399.240 737.607'
    '-823.843 4400.000 737.607'
    '-816.631 4408.490 737.966'
    '-809.060 4418.230 738.023'
    '-809.060 4418.230 738.023'
    '-775.389 4457.150 735.325'
    '-763.279 4559.600 726.677'
)
$original | Set-Content "$out/original.txt" -Encoding ascii
$baselineValues = for ($i = 0; $i -lt $original.Count; ++$i) {
    "(272369, $i, $($original[$i].Replace(' ', ', ')), 97512)"
}
$fixture = @"
CREATE TEMPORARY TABLE kula_spawns LIKE creature;
INSERT INTO kula_spawns SELECT * FROM creature WHERE guid IN (272369,58922);
CREATE TEMPORARY TABLE creature LIKE kula_spawns;
INSERT INTO creature SELECT * FROM kula_spawns;
UPDATE creature SET position_z=738.023 WHERE guid=272369;
CREATE TEMPORARY TABLE kula_paths LIKE waypoint_data;
CREATE TEMPORARY TABLE waypoint_data LIKE kula_paths;
INSERT INTO waypoint_data (id,point,position_x,position_y,position_z,entry) VALUES $($baselineValues -join ',');
INSERT INTO waypoint_data (id,point,position_x,position_y,position_z,action,entry) VALUES (58922,0,5809,576,653,117,32711);
$migrationText
CREATE TEMPORARY TABLE first_pass LIKE waypoint_data;
INSERT INTO first_pass SELECT * FROM waypoint_data;
$migrationText
SELECT 'binding', COUNT(*) FROM creature_addon WHERE path_id=272369 AND guid=272369;
SELECT 'spawn', COUNT(*) FROM creature c JOIN waypoint_data w ON w.id=272369 AND w.point=0 AND w.position_x=c.position_x AND w.position_y=c.position_y AND w.position_z=c.position_z WHERE c.guid=272369 AND c.id=97512 AND c.map=1220 AND c.MovementType=2;
SELECT 'same-on-replay', COUNT(*) FROM waypoint_data a JOIN first_pass b USING (id,point) WHERE a.position_x=b.position_x AND a.position_y=b.position_y AND a.position_z=b.position_z AND a.entry=b.entry AND a.action=b.action AND a.move_type=b.move_type;
SELECT 'old-city', COUNT(*) FROM creature a JOIN kula_spawns b USING(guid) WHERE a.guid=58922 AND a.map=571 AND a.position_z=b.position_z AND a.MovementType=b.MovementType;
SELECT 'old-city-path', COUNT(*) FROM waypoint_data WHERE id=58922 AND position_z=653 AND action=117 AND entry=32711;
SELECT 'new-route', COUNT(*) FROM waypoint_data WHERE id=272369 AND point BETWEEN 0 AND 29 AND entry=97512 AND delay=0 AND move_type=0 AND action=0;
SELECT position_x,position_y,position_z FROM waypoint_data WHERE id=272369 ORDER BY point;
"@
$info = [regex]::Match((Get-Content "$runtime/worldserver.conf" -Raw), '(?m)^\s*WorldDatabaseInfo\s*=\s*"([^"]+)"').Groups[1].Value.Split(';')
$previousPassword = $env:MYSQL_PWD
try {
    $env:MYSQL_PWD = $info[3]
    $rows = @($fixture | & 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mariadb.exe' --protocol=tcp "--host=$($info[0])" "--port=$($info[1])" "--user=$($info[2])" "--database=$($info[4])" --batch --raw --skip-column-names)
    if ($LASTEXITCODE) { throw 'Kula migration fixture failed' }
} finally { $env:MYSQL_PWD = $previousPassword }
$expected = @('binding 1', 'spawn 1', 'same-on-replay 31', 'old-city 1', 'old-city-path 1', 'new-route 30') |
    ForEach-Object { $_.Replace(' ', [string][char]9) }
if ($rows.Count -ne 36 -or ($rows[0..5] -join '|') -ne ($expected -join '|')) {
    throw "Unexpected Kula migration result: $($rows[0..5] -join ', ')"
}
$rows[6..35] | Set-Content "$out/repaired.txt" -Encoding ascii

[xml]$project = Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group = $project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$include = $group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object { $_ -and $_ -notlike '%*' }
$include += @('src/common/Collision/Management', 'src/common/Collision', 'dep/g3dlite/include', 'dep/recastnavigation/Detour/Include') | ForEach-Object { Join-Path $repo $_ }
$include += 'C:/local/boost_1_85_0', 'C:/wamp64/bin/mysql/mysql5.7.44/include', 'C:/Program Files/OpenSSL-Win64/include'
$libs = $group.Link.AdditionalDependencies.Split(';') | Where-Object { $_ -and $_ -notlike '%*' } | ForEach-Object {
    if ($_ -like '..*') { [IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_)) } else { $_ }
}
$programFilesX86 = [Environment]::GetEnvironmentVariable('ProgramFiles(x86)')
$vs = & "$programFilesX86/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compiler = @('/nologo', '/EHsc', '/std:c++17', '/MD', '/DNOMINMAX', '/DWIN32_LEAN_AND_MEAN', '/D_WIN32_WINNT=0x0601', '/DBOOST_ALL_NO_LIB')
$compiler += $include | ForEach-Object { '/I"' + $_ + '"' }
$compiler += '"' + $repo + '/tools/tests/DalaranKulaPatrolGeometry.cpp"'
$compiler += '/Fe"' + $out + '/KulaPatrol.exe"', '/Fo"' + $out + '/KulaPatrol.obj"', '/link'
$compiler += $libs | ForEach-Object { '"' + $_ + '"' }
[IO.File]::WriteAllLines("$out/build.cmd", @('@echo off', ('call "{0}/VC/Auxiliary/Build/vcvars64.bat" >nul' -f $vs), 'if errorlevel 1 exit /b 1', ('cl ' + ($compiler -join ' ')), 'exit /b %errorlevel%'))
& "$out/build.cmd"
if ($LASTEXITCODE) { throw 'Kula geometry probe build failed' }

$previousPath = $env:PATH
try {
    $env:PATH = "$runtime;$previousPath"
    & "$out/KulaPatrol.exe" $runtime "$out/original.txt" > "$out/baseline.stdout" 2> "$out/baseline.stderr"
    $baselineExit = $LASTEXITCODE
    $baselineErrors = Get-Content "$out/baseline.stderr" -Raw
    if ($baselineExit -ne 1 -or @('Unsafe link 4', 'Unsafe link 5', 'Unsafe link 6').Where({ $baselineErrors -notmatch $_ }).Count) {
        throw 'Original straight-route failure was not reproduced'
    }
    & "$out/KulaPatrol.exe" $runtime "$out/repaired.txt" > "$out/repaired.stdout" 2> "$out/repaired.stderr"
    if ($LASTEXITCODE -or (Get-Item "$out/repaired.stderr").Length) {
        Get-Content "$out/repaired.stderr"
        throw 'Repaired route crosses the street collision surface'
    }
    Get-Content "$out/repaired.stdout"
} finally { $env:PATH = $previousPath }
'PASS: baseline reproduced; repaired spawn and cyclic route pass; repeated migration and old Dalaran data are stable.'
