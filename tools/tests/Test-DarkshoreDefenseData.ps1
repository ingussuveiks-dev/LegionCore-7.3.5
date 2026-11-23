param(
    [string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/darkshore-defense-data",
    [switch]$VerifyLiveData
)

$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out = [IO.Path]::GetFullPath($OutputDirectory)
$migration = Get-Content "$repo/sql/updates/world/2026_11_23_12_darkshore_defense_map_and_summon.sql" -Raw
$columns = 'QuestID,BlobIndex,Idx1,ObjectiveIndex,QuestObjectiveID,QuestObjectID,MapID,WorldMapAreaId,Floor,Priority,Flags,WorldEffectID,PlayerConditionID,WoDUnk1,AlwaysAllowMergingBlobs'

& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
    $ordered = 'FROM quest_poi WHERE QuestID=13885 ORDER BY Idx1,BlobIndex'
    $query = @"
SELECT 'live',$columns $ordered;
SELECT 'live-target',target_map,target_position_x,target_position_y,target_position_z,target_orientation FROM spell_target_position WHERE id=65372;
CREATE TEMPORARY TABLE saved_darkshore LIKE quest_poi;
INSERT INTO saved_darkshore SELECT * FROM quest_poi WHERE QuestID IN(13885,13925);
CREATE TEMPORARY TABLE quest_poi LIKE saved_darkshore;
INSERT INTO quest_poi SELECT * FROM saved_darkshore;
CREATE TEMPORARY TABLE saved_spell_position LIKE spell_target_position;
CREATE TEMPORARY TABLE spell_target_position LIKE saved_spell_position;
INSERT INTO quest_poi ($columns,VerifiedBuild) VALUES
 (13885,1,1,0,267642,34323,1,42,0,0,3,0,0,0,0,23877),
 (13885,6,2,0,267643,34325,1,42,0,0,3,0,0,0,0,23877),
 (13885,5,3,0,267643,34325,1,42,0,0,3,0,0,0,0,23877),
 (13885,4,4,0,267643,34325,1,42,0,0,1,0,0,0,0,23877),
 (13885,2,5,0,267644,34324,1,42,0,0,1,0,0,0,0,23877)
ON DUPLICATE KEY UPDATE QuestID=VALUES(QuestID);
SELECT 'before',$columns $ordered;
SELECT 'retained',$columns FROM quest_poi
 WHERE QuestID=13885 AND (VerifiedBuild=26124 OR Idx1=0) ORDER BY Idx1,BlobIndex;
SELECT 'neighbour-before',$columns FROM quest_poi WHERE QuestID=13925 ORDER BY Idx1,BlobIndex;
SELECT 'points-before',QuestID,Idx1,X,Y FROM quest_poi_points WHERE QuestID=13885 ORDER BY Idx1,Idx2;
$migration
SELECT 'repaired',$columns $ordered;
SELECT 'target',target_map,target_position_x,target_position_y,target_position_z,target_orientation FROM spell_target_position WHERE id=65372;
$migration
SELECT 'rerun',$columns $ordered;
SELECT 'neighbour-after',$columns FROM quest_poi WHERE QuestID=13925 ORDER BY Idx1,BlobIndex;
SELECT 'points-after',QuestID,Idx1,X,Y FROM quest_poi_points WHERE QuestID=13885 ORDER BY Idx1,Idx2;
UPDATE spell_target_position SET target_orientation=2.5 WHERE id=65372;
INSERT INTO quest_poi VALUES (13885,5,3,0,267643,34325,1,42,0,0,3,0,7317,0,0,23877);
DELETE FROM quest_poi WHERE QuestID=13885 AND Idx1=4 AND VerifiedBuild=26124;
INSERT INTO quest_poi VALUES (13885,4,4,0,267643,34325,1,42,0,0,1,0,0,0,0,23877);
$migration
SELECT 'custom-target',target_orientation FROM spell_target_position WHERE id=65372;
SELECT 'custom-legacy',COUNT(*) FROM quest_poi WHERE QuestID=13885 AND (PlayerConditionID=7317 OR (Idx1=4 AND VerifiedBuild=23877));
"@
    $connection = (Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object { $_ -match '^WorldDatabaseInfo\s*=' }).Split('"')[1].Split(';')
    $previous = $env:MYSQL_PWD
    try {
        $env:MYSQL_PWD = $connection[3]
        $rows = @($query | & 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe' --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
        if ($LASTEXITCODE) { throw 'Darkshore data query failed' }
    }
    finally { $env:MYSQL_PWD = $previous }

    function Tagged([string]$tag) {
        $rows | Where-Object { $_ -like "$tag`t*" } | ForEach-Object { ($_ -split "`t", 2)[1] }
    }
    $before = @(Tagged 'before')
    $repaired = @(Tagged 'repaired')
    $points = @(Tagged 'points-after')
    if ($before.Count -ne 12 -or $repaired.Count -ne 7 -or $points.Count -ne 31 -or
        ($repaired -join '|') -cne ((Tagged 'retained') -join '|') -or
        ($repaired -join '|') -cne ((Tagged 'rerun') -join '|') -or
        ($points -join '|') -cne ((Tagged 'points-before') -join '|') -or
        ((Tagged 'neighbour-before') -join '|') -cne ((Tagged 'neighbour-after') -join '|') -or
        ((Tagged 'target') -join '|') -cne "1`t4961.64`t130.561`t70.5063`t0.957487" -or
        "custom-target`t2.5" -notin $rows -or "custom-legacy`t2" -notin $rows) {
        throw 'Darkshore migration, preservation or rerun check failed'
    }
    if ($VerifyLiveData -and (((Tagged 'live') -join '|') -cne ($repaired -join '|') -or
        ((Tagged 'live-target') -join '|') -cne ((Tagged 'target') -join '|'))) {
        throw 'Darkshore live POIs or summon position differ from the tested data'
    }
    'PASS Darkshore shadow migration, custom and missing counterpart guards, unchanged points and original summon destination'

    [IO.File]::WriteAllLines("$out/before.tsv", [string[]]$before)
    [IO.File]::WriteAllLines("$out/after.tsv", [string[]]$repaired)
    [IO.File]::WriteAllLines("$out/points.tsv", [string[]]$points)
    & "$PSScriptRoot/Export-QuestPoiFixture.ps1" -OutputDirectory $out
    $vs = & "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    [IO.File]::WriteAllText("$out/run.cmd", @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cd /d "$out"
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I"$out" "$PSScriptRoot/VrektPoiTest.cpp" /Fe:darkshore.exe /Fo:darkshore.obj /Fd:darkshore.pdb /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
darkshore.exe before.tsv after.tsv points.tsv darkshore
exit /b %errorlevel%
"@)
    & "$out/run.cmd"
    if ($LASTEXITCODE) { throw 'Native Darkshore POI test failed' }
}
