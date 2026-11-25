param(
    [string]$MigrationPath = "$PSScriptRoot/../../sql/updates/world/2026_11_25_13_artifact_quest_map_points.sql",
    [string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/artifact-quest-map-points",
    [switch]$VerifyLiveData
)

$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$runtime = Join-Path $repo 'build-extractors/bin/Release'
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
. "$PSScriptRoot/ServerDiagnosticLines.ps1"

function Get-Snapshot {
    @{
        Logs = @(Get-Item "$runtime/logs/Server.log","$runtime/logs/DBErrors.log" | Select-Object FullName,Length,LastWriteTimeUtc)
        Crashes = @(Get-ChildItem "$runtime/Crashes","$OutputDirectory/Crashes" -File -ErrorAction SilentlyContinue | Select-Object FullName,Length,LastWriteTimeUtc)
    }
}

$migration = Get-Content -LiteralPath $MigrationPath -Raw
$routes = Get-Content "$repo/docs/audits/artifact-acquisition-worklist-2026-10-23.json" -Raw | ConvertFrom-Json
$quests = @($routes.routes.quests.id | Sort-Object -Unique)
if ($routes.routes.Count -ne 36 -or $quests.Count -ne 188) { throw 'Unexpected acquisition audit scope' }
$ids = ($quests + 46744) -join ','
$settings = Get-Content "$runtime/worldserver.conf" -Raw
$parts = [regex]::Match($settings, '(?m)^\s*WorldDatabaseInfo\s*=\s*"([^"\r\n]+)"').Groups[1].Value.Split(';')
if ($parts.Length -ne 5) { throw 'Cannot read world database connection settings' }

$before = Get-Snapshot
$before | ConvertTo-Json -Depth 5 | Set-Content "$OutputDirectory/before.json"
$sql = @"
CREATE TEMPORARY TABLE original_artifact_poi LIKE quest_poi;
INSERT INTO original_artifact_poi SELECT * FROM quest_poi WHERE QuestID IN ($ids);
CREATE TEMPORARY TABLE original_artifact_points LIKE quest_poi_points;
INSERT INTO original_artifact_points SELECT * FROM quest_poi_points WHERE QuestID IN ($ids);
CREATE TEMPORARY TABLE quest_poi LIKE original_artifact_poi;
CREATE TEMPORARY TABLE quest_poi_points LIKE original_artifact_points;
INSERT INTO quest_poi SELECT * FROM original_artifact_poi;
INSERT INTO quest_poi_points SELECT * FROM original_artifact_points;

$migration
SELECT 'added_definitions', (SELECT COUNT(*) FROM quest_poi) - (SELECT COUNT(*) FROM original_artifact_poi);
SELECT 'added_points', (SELECT COUNT(*) FROM quest_poi_points) - (SELECT COUNT(*) FROM original_artifact_points);
SELECT 'lost_old_points', COUNT(*) FROM original_artifact_points o LEFT JOIN quest_poi_points p
 ON p.QuestID=o.QuestID AND p.Idx1=o.Idx1 AND p.Idx2=o.Idx2 AND p.X=o.X AND p.Y=o.Y AND p.VerifiedBuild<=>o.VerifiedBuild
 WHERE p.QuestID IS NULL;
SELECT 'lost_old_definitions', COUNT(*) FROM original_artifact_poi o LEFT JOIN quest_poi p
 ON p.QuestID=o.QuestID AND p.BlobIndex=o.BlobIndex AND p.Idx1=o.Idx1 WHERE p.QuestID IS NULL;
SELECT 'tirasfal', WorldMapAreaId FROM quest_poi WHERE QuestID=42120 AND BlobIndex=0 AND Idx1=5;
SELECT 'talonclaw', COUNT(*) FROM quest_poi_points WHERE QuestID=40385 AND Idx1=7;
SELECT 'sorrow_hill', COUNT(*) FROM quest_poi b JOIN quest_poi_points p USING (QuestID,Idx1)
 WHERE b.QuestID=42772 AND b.QuestObjectiveID=284747 AND b.WorldMapAreaId=22;
SELECT 'separated', COUNT(*) FROM quest_poi b JOIN quest_poi_points p USING (QuestID,Idx1)
 WHERE b.QuestID=39247 AND b.BlobIndex=3 AND b.Idx1=10 AND b.MapID=1494 AND p.X=1288 AND p.Y=-261;
SELECT 'original_sibling', COUNT(*) FROM quest_poi b JOIN quest_poi_points p USING (QuestID,Idx1)
 WHERE b.QuestID=39247 AND b.Idx1=6 AND p.X=-845 AND p.Y=4254;
SELECT 'scope', COUNT(DISTINCT b.QuestID) FROM quest_poi b JOIN quest_poi_points p USING (QuestID,Idx1)
 WHERE b.QuestID IN ($($quests -join ','));
SELECT 'first-definition', p.* FROM quest_poi p ORDER BY QuestID,Idx1,BlobIndex;
SELECT 'first-points', p.* FROM quest_poi_points p ORDER BY QuestID,Idx1,Idx2;

$migration
SELECT 'repeat-definition', p.* FROM quest_poi p ORDER BY QuestID,Idx1,BlobIndex;
SELECT 'repeat-points', p.* FROM quest_poi_points p ORDER BY QuestID,Idx1,Idx2;

DELETE FROM quest_poi;
DELETE FROM quest_poi_points;
INSERT INTO quest_poi SELECT * FROM original_artifact_poi;
INSERT INTO quest_poi_points SELECT * FROM original_artifact_points;
INSERT INTO quest_poi_points VALUES (40385,7,0,123,456,0),(39247,10,0,789,654,0);
UPDATE quest_poi SET PlayerConditionID=PlayerConditionID+1 WHERE QuestID=40385 AND Idx1=8;
INSERT INTO quest_poi SELECT QuestID, BlobIndex+10, Idx1, ObjectiveIndex, QuestObjectiveID, QuestObjectID,
 MapID, WorldMapAreaId, Floor, Priority, Flags, WorldEffectID, PlayerConditionID, WoDUnk1, AlwaysAllowMergingBlobs, VerifiedBuild
 FROM original_artifact_poi WHERE QuestID=40385 AND Idx1=9;
UPDATE quest_poi_points SET X=X+1 WHERE QuestID=40710 AND Idx1=1 AND Idx2=0;
INSERT INTO quest_poi SELECT 41113,0,0,32,0,0,1220,1014,0,0,0,0,0,0,0,0;

$migration
SELECT 'custom', COUNT(*) FROM quest_poi_points WHERE QuestID=40385 AND Idx1=7 AND Idx2=0 AND X=123 AND Y=456;
SELECT 'custom_count', COUNT(*) FROM quest_poi_points WHERE QuestID=40385 AND Idx1=7;
SELECT 'changed_condition', COUNT(*) FROM quest_poi_points WHERE QuestID=40385 AND Idx1=8;
SELECT 'shared_empty_index', COUNT(*) FROM quest_poi_points WHERE QuestID=40385 AND Idx1=9;
SELECT 'orphan_reservation', COUNT(*) FROM quest_poi WHERE QuestID=39247 AND BlobIndex=3 AND Idx1=6;
SELECT 'changed_original', COUNT(*) FROM quest_poi WHERE QuestID=40710 AND BlobIndex=0 AND Idx1=1;
SELECT 'custom_definition', COUNT(*) FROM quest_poi WHERE QuestID=41113 AND Idx1=0 AND MapID=1220;
SELECT 'custom_definition_points', COUNT(*) FROM quest_poi_points WHERE QuestID=41113 AND Idx1=0;
"@
if ($VerifyLiveData) {
    $boundary = $migration.IndexOf('CREATE TEMPORARY TABLE artifact_poi_apply')
    if ($boundary -lt 0) { throw 'Cannot find migration evidence tables' }
    $expected = $migration.Substring(0,$boundary)
    $sql = @"
$expected
SELECT 'definitions', COUNT(*) FROM artifact_poi_plan a JOIN quest_poi b
 ON b.QuestID=a.QuestID AND b.BlobIndex=a.BlobIndex AND b.Idx1=a.TargetIdx1
 AND b.ObjectiveIndex=a.ObjectiveIndex AND b.QuestObjectiveID=a.QuestObjectiveID
 AND b.QuestObjectID=a.QuestObjectID AND b.MapID=a.MapID AND b.WorldMapAreaId=a.WorldMapAreaId
 AND b.Floor=a.Floor AND b.Priority=a.Priority AND b.Flags=a.Flags AND b.WorldEffectID=a.WorldEffectID
 AND b.PlayerConditionID=a.PlayerConditionID AND b.WoDUnk1=a.WoDUnk1 AND b.AlwaysAllowMergingBlobs=a.AlwaysAllowMergingBlobs;
SELECT 'coordinates', COUNT(*) FROM artifact_poi_coordinates e JOIN quest_poi_points p
 ON p.QuestID=e.QuestID AND p.Idx1=e.Idx1 AND p.Idx2=e.Idx2
 AND p.X=e.X AND p.Y=e.Y AND p.VerifiedBuild=e.VerifiedBuild;
SELECT 'tirasfal', WorldMapAreaId FROM quest_poi WHERE QuestID=42120 AND BlobIndex=0 AND Idx1=5;
SELECT 'scope', COUNT(DISTINCT b.QuestID) FROM quest_poi b JOIN quest_poi_points p USING (QuestID,Idx1)
 WHERE b.QuestID IN ($($quests -join ','));
ROLLBACK;
"@
}

$client = [Diagnostics.ProcessStartInfo]::new()
$client.FileName = 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mariadb.exe'
$client.UseShellExecute = $false
$client.RedirectStandardInput = $true
$client.RedirectStandardOutput = $true
$client.RedirectStandardError = $true
$client.Environment['MYSQL_PWD'] = $parts[3]
foreach ($argument in '--no-defaults','--protocol=TCP','--batch','--raw','--skip-column-names','-h',$parts[0],'-P',$parts[1],'-u',$parts[2],$parts[4]) {
    $client.ArgumentList.Add($argument)
}
$process = [Diagnostics.Process]::Start($client)
try {
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    $process.StandardInput.Write($sql)
    $process.StandardInput.Close()
    $process.WaitForExit()
    $output = $stdout.GetAwaiter().GetResult()
    $errors = $stderr.GetAwaiter().GetResult()
    [IO.File]::WriteAllText("$OutputDirectory/stdout.tsv",$output)
    [IO.File]::WriteAllText("$OutputDirectory/stderr.log",$errors)
    if ($process.ExitCode -ne 0) { throw "Shadow migration exited $($process.ExitCode). $errors" }
} finally {
    $process.Dispose()
    $after = Get-Snapshot
    $after | ConvertTo-Json -Depth 5 | Set-Content "$OutputDirectory/after.json"
    if (($before.Crashes | ConvertTo-Json -Compress) -ne ($after.Crashes | ConvertTo-Json -Compress)) {
        throw 'Crash file inventory changed during shadow verification'
    }
    foreach ($item in $before.Logs) {
        $stream = [IO.File]::Open($item.FullName,'Open','Read','ReadWrite')
        try {
            if ($stream.Length -lt $item.Length) { throw "Log was recreated during verification: $($item.FullName)" }
            [void]$stream.Seek($item.Length,'Begin')
            $reader = [IO.StreamReader]::new($stream)
            $tail = $reader.ReadToEnd()
            if ($tail -and ($item.FullName -like '*DBErrors.log' -or @(Get-ServerDiagnosticLines $tail).Count)) {
                [IO.File]::WriteAllText("$OutputDirectory/appended-$([IO.Path]::GetFileName($item.FullName))",$tail)
                throw "New server diagnostics during verification: $($item.FullName)"
            }
        } finally { $stream.Dispose() }
    }
}

$rows = $output -split '\r?\n'
if ($VerifyLiveData) {
    foreach ($expected in "definitions`t62","coordinates`t128","tirasfal`t20","scope`t188") {
        if ($expected -notin $rows) { throw "Missing live POI evidence: $expected" }
    }
    'PASS: live data matches all 62 planned definitions, 128 coordinates, the Tirisfal marker and 188-quest scope.'
    return
}
$checks = [ordered]@{
    added_definitions=10; added_points=128; lost_old_points=0; lost_old_definitions=34
    tirasfal=20; talonclaw=12; sorrow_hill=3; separated=1; scope=188
    custom=1; custom_count=1; changed_condition=0; shared_empty_index=0
    orphan_reservation=1; changed_original=1; custom_definition=1; custom_definition_points=0
}
foreach ($check in $checks.GetEnumerator()) {
    if ("$($check.Key)`t$($check.Value)" -notin $rows) { throw "Unexpected shadow result for $($check.Key)" }
}
if (@($rows | Where-Object { $_ -match "^original_sibling`t[1-9]" }).Count -ne 1) {
    throw 'Original coordinate sibling was lost'
}
function Get-Tagged([string]$Tag) {
    $rows | Where-Object { $_.StartsWith("$Tag`t") } | ForEach-Object { $_.Substring($Tag.Length + 1) }
}
foreach ($type in 'definition','points') {
    if (((Get-Tagged "first-$type") -join "`n") -cne ((Get-Tagged "repeat-$type") -join "`n")) {
        throw "Repeated migration modified $type"
    }
}
'PASS: 188 quests covered, 128 points added, 34 shared indexes split, no old coordinates lost, repeat and guard fixtures passed.'
