param(
    [string]$SqlFile = "$PSScriptRoot/../../sql/updates/world/2026_11_27_29_low_level_quest_auras.sql",
    [string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/low-level-quest-auras",
    [switch]$VerifyLiveData
)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$sql = Get-Content -LiteralPath $SqlFile -Raw
$readback = @'
SELECT spell, area, quest_start, quest_start_status, quest_end, quest_end_status, autocast
FROM spell_area WHERE spell IN (65602, 69744, 71388) ORDER BY spell;
'@
$expected = @('65602|141|14005|8|14005|8|1', '69744|16|24452|8|24452|8|0', '71388|4842|24904|64|24904|11|0')
$baseline = @('65602|0|14005|8|14005|66|1', '69744|0|24452|8|24452|66|1', $expected[2])
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $OutputDirectory -Test {
    $query = $readback
    $wanted = $expected
    if (!$VerifyLiveData) {
        $query = @"
CREATE TEMPORARY TABLE original_spell_area LIKE spell_area;
INSERT INTO original_spell_area SELECT * FROM spell_area WHERE spell IN (65602, 69744, 71388);
CREATE TEMPORARY TABLE spell_area LIKE original_spell_area;
INSERT INTO spell_area SELECT * FROM original_spell_area;
UPDATE spell_area SET area = 0, quest_end_status = 66, autocast = 1 WHERE spell IN (65602, 69744);
$readback
$sql
$readback
$sql
$readback
"@
        $wanted = $baseline + $expected + $expected
    }
    $config = (Get-Content -LiteralPath "$repo/build-extractors/bin/Release/worldserver.conf" |
        Where-Object { $_ -match '^WorldDatabaseInfo\s*=' }).Split('"')[1].Split(';')
    $previous = $env:MYSQL_PWD
    try {
        $env:MYSQL_PWD = $config[3]
        $rows = @($query | & 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mariadb.exe' --batch --raw --skip-column-names `
            --host=$($config[0]) --port=$($config[1]) --user=$($config[2]) --database=$($config[4]))
        if ($LASTEXITCODE) { throw 'Low-level quest aura SQL probe failed.' }
    } finally { $env:MYSQL_PWD = $previous }
    if ((($rows -replace '\t', '|') -join '/ ') -cne ($wanted -join '/ ')) {
        $rows
        throw 'Unexpected quest aura rows.'
    }
    if ($VerifyLiveData) { 'PASS live quest aura rows.' }
    else { 'PASS original and twice-applied quest aura rows.' }
}
