#requires -Version 7.0
param([Parameter(Mandatory)][string]$OutputDirectory, [switch]$VerifyLiveData)

$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out = [IO.Path]::GetFullPath($OutputDirectory)
$migrationName = '2026_11_28_10_early_smartai_continuations.sql'
$migration = Get-Content -Raw -LiteralPath "$repository/sql/updates/world/$migrationName"

& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/diagnostics" -Test {
    & "$PSScriptRoot/Invoke-WithTemporarySqlReader.ps1" -EvidenceDirectory "$out/reader" -Test {
        $setup = @'
CREATE TEMPORARY TABLE IF NOT EXISTS original_shape LIKE smart_scripts;
CREATE TEMPORARY TABLE smart_scripts LIKE original_shape;
INSERT INTO smart_scripts (entryorguid, source_type, id, link, event_type, event_flags,
    event_param1, event_param2, action_type, action_param1, action_param2, target_type, comment) VALUES
    (39261,0,0,0,4,1,0,0,21,0,0,1,'Stop Moving on Aggro'),
    (39261,0,0,1,4,1,0,0,21,0,0,1,'Stop Moving on Aggro'),
    (39261,0,1,2,4,1,0,0,11,6660,0,2,'Cast Shoot'),
    (39261,0,2,3,61,1,0,0,20,0,0,1,'Stop Melee Attack on Aggro'),
    (39261,0,3,0,61,1,0,0,22,1,0,1,'Set Phase 1 on Aggro'),
    (41763,0,0,1,54,0,0,0,5,20,0,1,'on spawn - play emote'),
    (41763,0,1,2,1,1,3000,3000,46,6,0,1,'3 sec timer - move forward'),
    (41763,0,2,0,61,0,0,0,41,3000,0,1,'link - despawn after 3 sec'),
    (45910,0,0,1,73,0,0,0,33,45910,0,7,'Smart Script'),
    (45910,0,1,2,61,0,0,0,1,1,3000,1,'Smart Script'),
    (45910,0,3,0,52,0,1,45910,41,4000,0,1,'Smart Script'),
    (45910,9,1,2,61,0,0,0,1,1,3000,1,'Different source');
'@
        $scope = 'source_type=0 AND ((entryorguid=39261 AND id<=3) OR (entryorguid=41763 AND id<=2) OR (entryorguid=45910 AND id<=3))'
        function Snapshot([string]$label, [string]$filter) {
            "SELECT '$label',smart_scripts.* FROM smart_scripts WHERE $filter ORDER BY entryorguid,source_type,id,link;`n"
        }

        $counterfactual = @'
UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=39261 AND source_type=0 AND id=0 AND link=1;
UPDATE smart_scripts SET event_param1=2999 WHERE entryorguid=41763 AND source_type=0 AND id=1;
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,action_type,comment)
VALUES (45910,0,2,61,72,'Local continuation');
'@

        $sql = $setup + (Snapshot baseline $scope) + $migration + (Snapshot repaired $scope) +
            $migration + (Snapshot repeated $scope) + (Snapshot other 'source_type=9') +
            'DROP TEMPORARY TABLE smart_scripts;' + $setup + $counterfactual +
            (Snapshot 'custom-before' $scope) + $migration + (Snapshot 'custom-after' $scope)
        if ($VerifyLiveData) {
            $sql += 'DROP TEMPORARY TABLE smart_scripts;' + (Snapshot live $scope) +
                "SELECT 'migration', hash, state FROM updates WHERE name='$migrationName';"
        }
        $sql += "SELECT 'DONE';"
        [IO.File]::WriteAllText("$out/replay.sql", $sql)
        $settings = (Select-String -LiteralPath "$repository/build-extractors/bin/Release/worldserver.conf" -Pattern '^WorldDatabaseInfo\s*=').Line.Split('"')[1].Split(';')
        $clientArgs = @('--protocol=TCP','--default-character-set=utf8mb4','--batch','--raw','--skip-column-names',
            "--host=$($settings[0])","--port=$($settings[1])","--database=$($settings[4])","--user=$env:CODEX_SQL_READER_USER")
        $oldPassword = $env:MYSQL_PWD
        try {
            $env:MYSQL_PWD = $env:CODEX_SQL_READER_PASSWORD
            $exit = & "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mariadb.exe' `
                -Arguments $clientArgs -InputFile "$out/replay.sql" -OutputFile "$out/replay.tsv" -ErrorFile "$out/replay.stderr"
        } finally { $env:MYSQL_PWD = $oldPassword }
        if ($exit -ne 0 -or (Get-Item -LiteralPath "$out/replay.stderr").Length) { throw 'MariaDB replay failed; see replay.stderr' }
        $actual = Get-Content -LiteralPath "$out/replay.tsv"
        if ($actual[-1] -cne 'DONE') { throw 'Incomplete MariaDB replay' }
        function Rows([string]$label) {
            $prefix = "$label`t"
            $actual | Where-Object { $_.StartsWith($prefix, [StringComparison]::Ordinal) } | ForEach-Object { $_.Substring($prefix.Length) }
        }

        # Independent expected result: one exact deletion and three column edits
        # against full rows (including all event, action and targeting fields).
        $baseline = @(Rows baseline)
        if ($baseline.Count -ne 11) { throw 'Fixture baseline changed' }
        $expected = foreach ($row in $baseline) {
            $cells = $row.Split("`t")
            $key = "$($cells[0])/$($cells[2])/$($cells[3])"
            switch ($key) {
                '39261/0/0' { break }
                '39261/1/2' { $cells[5] = '61'; $cells -join "`t" }
                '41763/0/1' { $cells[3] = '0'; $cells -join "`t" }
                '45910/1/2' { $cells[3] = '0'; $cells -join "`t" }
                default { $row }
            }
        }
        foreach ($label in @('repaired', 'repeated') + $(if ($VerifyLiveData) { @('live') } else { @() })) {
            if (((Rows $label) -join "`n") -cne ($expected -join "`n")) { throw "Full row or ordering mismatch: $label" }
        }
        if (((Rows 'custom-before') -join "`n") -cne ((Rows 'custom-after') -join "`n") -or
            (Rows other) -cnotmatch '^45910\s+9\s+1\s+2\s') { throw 'Different source or custom continuation was changed' }

        if ($VerifyLiveData) {
            $hash = [Convert]::ToHexString([Security.Cryptography.SHA1]::HashData([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n", "`n"))))
            if ((Rows migration) -cne "$hash`tRELEASED") { throw 'Live updater identity differs' }
        }
        'PASS original and exact repaired SmartAI rows, repeat application, different source and three custom-route refusals'
    }
}
