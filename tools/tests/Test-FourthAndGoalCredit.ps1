#requires -Version 7.0
param([Parameter(Mandatory)][string]$OutputDirectory, [switch]$VerifyLiveData)

$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out = [IO.Path]::GetFullPath($OutputDirectory)
$migrationName = '2026_11_28_18_fourth_and_goal_credit.sql'
$migration = Get-Content -Raw -LiteralPath "$repo/sql/updates/world/$migrationName"

& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/diagnostics" -Test {
    & "$PSScriptRoot/Invoke-WithTemporarySqlReader.ps1" -EvidenceDirectory "$out/reader" -Test {
        $setup = @'
CREATE TEMPORARY TABLE IF NOT EXISTS original_smart_shape LIKE smart_scripts;
CREATE TEMPORARY TABLE IF NOT EXISTS original_condition_shape LIKE conditions;
CREATE TEMPORARY TABLE smart_scripts LIKE original_smart_shape;
CREATE TEMPORARY TABLE conditions LIKE original_condition_shape;
INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_flags,event_param1,
    action_type,action_param1,action_param2,action_param3,target_type,target_x,target_y,target_z,target_o,comment) VALUES
    (37203,0,0,0,8,0,70052,33,37203,0,0,7,0,0,0,0,'credit'),
    (37203,0,0,1,8,1,70052,33,37203,0,0,7,0,0,0,0,'credit'),
    (37203,0,1,0,61,1,0,12,423070,1,300000,8,-8261.16,1486.31,63.6325,2.945,''),
    (37203,9,0,1,8,1,70052,33,37203,0,0,7,0,0,0,0,'other source');
'@
        function Snapshot([string]$label) {
            "SELECT '$label',smart_scripts.* FROM smart_scripts WHERE entryorguid=37203 ORDER BY source_type,id,link;`n"
        }
        $sql = $setup + (Snapshot baseline) + $migration + (Snapshot repaired) + $migration + (Snapshot repeated)
        $sql += 'DROP TEMPORARY TABLE smart_scripts, conditions;' + $setup
        $sql += 'UPDATE smart_scripts SET event_param3=1000 WHERE entryorguid=37203 AND source_type=0 AND id=0 AND link=1;'
        $sql += (Snapshot 'custom-before') + $migration + (Snapshot 'custom-after')
        $sql += 'DROP TEMPORARY TABLE smart_scripts, conditions;' + $setup
        $sql += 'INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ConditionTypeOrReference,ConditionValue1) VALUES (22,2,37203,0,9,24503);'
        $sql += (Snapshot 'conditional-before') + $migration + (Snapshot 'conditional-after')
        if ($VerifyLiveData) {
            $sql += 'DROP TEMPORARY TABLE smart_scripts, conditions;' + (Snapshot live)
            $sql += "SELECT 'migration', hash, state FROM updates WHERE name='$migrationName';"
            $sql += "SELECT 'objective', QuestID, ObjectID, Amount FROM quest_objectives WHERE QuestID IN (24503,28414) AND Type=0 ORDER BY QuestID,ID;"
            $sql += "SELECT 'scene', entry, COUNT(*) FROM waypoints WHERE entry=423070 GROUP BY entry;"
        }
        $sql += "SELECT 'DONE';"
        [IO.File]::WriteAllText("$out/replay.sql", $sql)
        $settings = (Select-String -LiteralPath "$repo/build-extractors/bin/Release/worldserver.conf" -Pattern '^WorldDatabaseInfo\s*=').Line.Split('"')[1].Split(';')
        $args = @('--protocol=TCP','--default-character-set=utf8mb4','--batch','--raw','--skip-column-names',
            "--host=$($settings[0])","--port=$($settings[1])","--database=$($settings[4])","--user=$env:CODEX_SQL_READER_USER")
        $oldPassword = $env:MYSQL_PWD
        try {
            $env:MYSQL_PWD = $env:CODEX_SQL_READER_PASSWORD
            $exitCode = & "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mariadb.exe' `
                -Arguments $args -InputFile "$out/replay.sql" -OutputFile "$out/replay.tsv" -ErrorFile "$out/replay.stderr"
        } finally { $env:MYSQL_PWD = $oldPassword }
        if ($exitCode -ne 0 -or (Get-Item -LiteralPath "$out/replay.stderr").Length) { throw 'MariaDB replay failed, see replay.stderr' }
        $actual = Get-Content -LiteralPath "$out/replay.tsv"
        if ($actual[-1] -cne 'DONE') { throw 'Incomplete MariaDB replay' }
        function Rows([string]$label) {
            $prefix = "$label`t"
            $actual | Where-Object { $_.StartsWith($prefix, [StringComparison]::Ordinal) } | ForEach-Object { $_.Substring($prefix.Length) }
        }

        # Compare complete rows so native summon coordinates, lifetime and target survive.
        $baseline = @(Rows baseline)
        if ($baseline.Count -ne 4) { throw 'Fixture baseline changed' }
        $expected = foreach ($row in $baseline) {
            $cells = $row.Split("`t")
            if ($cells[1] -eq '0' -and $cells[2] -eq '0' -and $cells[3] -eq '1') { continue }
            if ($cells[1] -eq '0' -and $cells[2] -eq '1') {
                $cells[5] = '8'
                $cells[9] = '70052'
                $cells[-1] = 'Fourth And Goal Target - On Footbomb Hit - Summon Scene Actor (No Repeat)'
            }
            $cells -join "`t"
        }
        if ($expected.Count -ne 3 -or $expected[0] -cne $baseline[0]) { throw 'Unexpected expected rows' }
        foreach ($state in @('repaired','repeated')) {
            if (((Rows $state) -join "`n") -cne ($expected -join "`n")) { throw "Row mismatch after $state application" }
        }
        foreach ($state in @('custom','conditional')) {
            if (((Rows "$state-before") -join "`n") -cne ((Rows "$state-after") -join "`n")) { throw "Migration altered $state data" }
        }

        if ($VerifyLiveData) {
            $hash = [Convert]::ToHexString([Security.Cryptography.SHA1]::HashData([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n", "`n"))))
            if ((Rows migration) -cne "$hash`tRELEASED") { throw 'Live updater identity differs' }
            $expectedLive = $expected | Where-Object { $_.StartsWith("37203`t0`t", [StringComparison]::Ordinal) }
            $actualLive = Rows live | Where-Object { $_.StartsWith("37203`t0`t", [StringComparison]::Ordinal) }
            if (($actualLive -join "`n") -cne ($expectedLive -join "`n")) { throw 'Live SmartAI rows differ' }
            if (((Rows objective) -join "`n") -cne "24503`t37203`t1`n28414`t37203`t1") { throw 'Live quest objective changed' }
            if ((Rows scene) -cne "423070`t7") { throw 'Live scene actor path differs' }
        }
        'PASS exact Fourth and Goal rows, repeat migration, custom timing and event condition guards'
    }
}
