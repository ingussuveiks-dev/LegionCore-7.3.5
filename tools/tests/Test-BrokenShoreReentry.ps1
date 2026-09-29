param([string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/broken-shore-reentry")
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null

function Export-Function([string]$Path, [string]$Marker, [string]$Name) {
    $source = [IO.File]::ReadAllText((Join-Path $repo $Path))
    $start = $source.IndexOf($Marker)
    if ($start -lt 0) { throw "Cannot find $Marker" }
    $open = $source.IndexOf('{', $start)
    $depth = 1
    $end = $open + 1
    while ($depth -gt 0 -and $end -lt $source.Length) {
        if ($source[$end] -eq '{') { ++$depth }
        if ($source[$end] -eq '}') { --$depth }
        ++$end
    }
    if ($depth -ne 0) { throw "Unbalanced function $Marker" }
    [IO.File]::WriteAllText((Join-Path $output $Name), $source.Substring($start, $end - $start))
}
Export-Function 'src/server/game/Maps/MapManager.cpp' 'bool MapManager::CanPlayerEnter(' 'CanPlayerEnter.inc'
Export-Function 'src/server/game/Groups/Group.cpp' 'void Group::LoadGroupFromDB(' 'LoadGroup.inc'
Export-Function 'src/server/scripts/Scenario/BrokenIslands/broken_islands.cpp' 'namespace' 'BrokenShoreReturn.inc'
Export-Function 'src/server/scripts/Scenario/BrokenIslands/instance_broken_islands.cpp' 'void RestoreScenarioEntrance(' 'RestoreScenarioEntrance.inc'
Export-Function 'src/server/game/Entities/Creature/GossipDef.cpp' 'void GossipMenu::AddMenuItem(int32' 'GossipMenuAdd.inc'
Export-Function 'src/server/game/AI/SmartScripts/SmartScript.cpp' 'case SMART_EVENT_GOSSIP_SELECT:' 'SmartGossipSelect.inc'

$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$commands = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/BrokenShoreReentryTest.cpp" /Fe"$output/BrokenShoreReentryTest.exe" /Fo"$output/BrokenShoreReentryTest.obj"
if errorlevel 1 exit /b 1
"$output/BrokenShoreReentryTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd", $commands)
& "$output/run.cmd"
if ($LASTEXITCODE -ne 0) { throw "Broken Shore reentry regression failed: $LASTEXITCODE" }
'PASS: production login checks, LFG group restoration, return gossip and stage entrance recovery.'

# Exercise the actual startup cleanup SQL. Temporary tables shadow the live
# names in this connection only; no character/group data is changed.
$groupSource = [IO.File]::ReadAllText("$repo/src/server/game/Groups/GroupMgr.cpp")
$sql = [regex]::Matches($groupSource, 'CharacterDatabase.DirectExecute\("([^"]*)"(?:\s*"([^"]*)")?\);') |
    ForEach-Object { $_.Groups[1].Value + $_.Groups[2].Value }
if ($sql.Count -ne 6) { throw "Unexpected group cleanup statement count: $($sql.Count)" }
$fixture = @"
CREATE TEMPORARY TABLE characters (guid BIGINT PRIMARY KEY);
CREATE TEMPORARY TABLE groups (guid INT PRIMARY KEY, leaderGuid BIGINT, groupType INT);
CREATE TEMPORARY TABLE group_member (guid INT, memberGuid BIGINT);
CREATE TEMPORARY TABLE group_instance (guid INT);
CREATE TEMPORARY TABLE lfg_data (guid INT);
INSERT INTO characters VALUES (1),(2),(3),(4),(5),(6);
INSERT INTO groups VALUES (10,1,12),(11,2,0),(12,3,12),(13,4,0),(14,99,12);
INSERT INTO group_member VALUES (10,1),(11,2),(13,4),(13,5),(14,99);
INSERT INTO group_instance VALUES (10),(11),(12),(13),(14),(99);
INSERT INTO lfg_data VALUES (10),(11),(12),(13),(14),(99);
$($sql -join ";`n");
SELECT 'groups', GROUP_CONCAT(guid ORDER BY guid) FROM groups;
SELECT 'members', GROUP_CONCAT(guid ORDER BY guid) FROM group_member;
SELECT 'instances', GROUP_CONCAT(guid ORDER BY guid) FROM group_instance;
SELECT 'lfg', GROUP_CONCAT(guid ORDER BY guid) FROM lfg_data;
"@
$runtime = Join-Path $repo 'build-extractors/bin/Release'
$connection = (Get-Content "$runtime/worldserver.conf" | Where-Object { $_ -match '^CharacterDatabaseInfo\s*=' }).Split('"')[1].Split(';')
$mysql = Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe | Select-Object -First 1 -ExpandProperty FullName
$previousPassword = $env:MYSQL_PWD
try {
    $env:MYSQL_PWD = $connection[3]
    $rows = & $mysql --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names --execute=$fixture
    if ($LASTEXITCODE -ne 0) { throw 'Temporary group cleanup fixture failed' }
    $expected = @("groups`t10,13", "members`t10,13,13", "instances`t10,13", "lfg`t10,13")
    if (($rows -join "`n") -ne ($expected -join "`n")) { throw "Unexpected cleanup results: $($rows -join ', ')" }
} finally { $env:MYSQL_PWD = $previousPassword }
'PASS: startup preserves a one-member LFG group and its data, and removes invalid/empty/ordinary solo groups.'
