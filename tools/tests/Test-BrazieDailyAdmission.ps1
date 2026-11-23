param(
    [string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/brazie-daily-admission",
    [switch]$VerifyLiveData
)

$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out = [IO.Path]::GetFullPath($OutputDirectory)
$migration = Get-Content "$repo/sql/updates/world/2026_11_23_11_brazie_daily_prerequisite.sql" -Raw

& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
    $quests = '28617,28733,28744,28747,28748,28751'
    $select = "SELECT q.ID,q.Flags,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN ($quests) ORDER BY q.ID;"
    $query = @"
SELECT 'live';
$select
CREATE TEMPORARY TABLE brazie_source LIKE quest_template_addon;
INSERT INTO brazie_source SELECT * FROM quest_template_addon WHERE ID IN ($quests);
CREATE TEMPORARY TABLE quest_template_addon LIKE brazie_source;
INSERT INTO quest_template_addon SELECT * FROM brazie_source;
UPDATE quest_template_addon SET PrevQuestID=0 WHERE ID=28751 AND PrevQuestID=28748;
SELECT 'baseline';
$select
$migration
SELECT 'repaired';
$select
$migration
SELECT 'rerun';
$select
UPDATE quest_template_addon SET PrevQuestID=28747 WHERE ID=28751;
$migration
SELECT 'custom';
$select
"@
    $config = Get-Content -LiteralPath "$repo/build-extractors/bin/Release/worldserver.conf"
    $connection = ($config | Where-Object { $_ -match '^WorldDatabaseInfo\s*=' }).Split('"')[1].Split(';')
    $old = $env:MYSQL_PWD
    try {
        $env:MYSQL_PWD = $connection[3]
        $rows = @($query | & 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe' --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
        if ($LASTEXITCODE) { throw 'Could not read Brazie admission data' }
    }
    finally {
        $env:MYSQL_PWD = $old
    }

    $sections = @{}
    $name = ''
    foreach ($row in $rows) {
        if ($row -in 'live','baseline','repaired','rerun','custom') {
            $name = $row
            $sections[$name] = @()
        }
        elseif ($name) {
            $sections[$name] += $row
        }
    }
    foreach ($name in 'live','baseline','repaired','rerun','custom') {
        if ($sections[$name].Count -ne 6) { throw "Unexpected quest data in $name" }
    }
    $before = $sections.baseline -join "`n"
    $after = $sections.repaired -join "`n"
    $expected = $before.Replace("28751`t5247040`t0`t0`t0","28751`t5247040`t28748`t0`t0")
    if ($before -ceq $expected -or $after -cne $expected -or ($sections.rerun -join "`n") -cne $after) {
        throw 'Migration changed something outside the daily prerequisite or failed to repeat cleanly'
    }
    if (($sections.custom -join "`n") -cne $after.Replace("28751`t5247040`t28748`t0`t0","28751`t5247040`t28747`t0`t0")) {
        throw 'The migration overwrote a custom prerequisite'
    }
    if ($VerifyLiveData -and ($sections.live -join "`n") -cne $after) {
        throw 'Live Brazie prerequisite differs from the tested rows'
    }
    [IO.File]::WriteAllLines("$out/baseline.tsv", [string[]]$sections.baseline)
    [IO.File]::WriteAllLines("$out/repaired.tsv", [string[]]$sections.repaired)
    'PASS MariaDB shadow migration, rerun, custom prerequisite and other five quest links'

    $source = Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
    $start = $source.IndexOf('        if (qinfo->PrevQuestID)')
    $end = $source.IndexOf('        if (qinfo->LimitTime)', $start)
    if ($start -lt 0 -or $end -lt 0) { throw 'Missing native quest link loader' }
    [IO.File]::WriteAllText("$out/QuestLinks.inc", $source.Substring($start, $end - $start))
    $source = Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
    $admission = foreach ($pair in @(
        @('bool Player::SatisfyQuestPreviousQuest(', 'bool Player::SatisfyQuestClass('),
        @('bool Player::SatisfyQuestDay(', 'bool Player::SatisfyQuestWeek(')
    )) {
        $start = $source.IndexOf($pair[0])
        $end = $source.IndexOf($pair[1], $start)
        if ($start -lt 0 -or $end -lt 0) { throw 'Missing native quest admission method' }
        $source.Substring($start, $end - $start)
    }
    [IO.File]::WriteAllText("$out/QuestAdmission.inc", ($admission -join "`n"))
    $vs = & "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vs) { throw 'Visual C++ compiler not found' }
    [IO.File]::WriteAllText("$out/run.cmd", @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cd /d "$out"
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I"$out" "$PSScriptRoot/BrazieDailyAdmissionTest.cpp" /Fe:brazie.exe /Fo:brazie.obj /Fd:brazie.pdb /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
brazie.exe baseline.tsv baseline
if errorlevel 1 exit /b 1
brazie.exe repaired.tsv repaired
exit /b %errorlevel%
"@)
    & "$out/run.cmd"
    if ($LASTEXITCODE) { throw 'Brazie daily admission regression failed' }
}
