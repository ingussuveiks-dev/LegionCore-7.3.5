param([Parameter(Mandatory)][string]$OutputDirectory, [switch]$Baseline)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot/../..").Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force $output | Out-Null

function Read-Source([string]$path) {
    if ($Baseline) {
        $lines = & git -C $root show "54d44e8d3:$path"
        if ($LASTEXITCODE) { throw "Cannot read baseline $path" }
        return $lines -join "`n"
    }
    return Get-Content -LiteralPath "$root/$path" -Raw
}

function Get-Method([string]$source, [string]$name) {
    $start = $source.IndexOf($name, [StringComparison]::Ordinal)
    if ($start -lt 0) { throw "Missing production method $name" }
    $open = $source.IndexOf('{', $start)
    $depth = 1
    $end = $open + 1
    while ($depth -and $end -lt $source.Length) {
        if ($source[$end] -eq '{') { ++$depth }
        if ($source[$end] -eq '}') { --$depth }
        ++$end
    }
    if ($depth) { throw "Unbalanced production method $name" }
    return $source.Substring($start, $end - $start)
}

$queue = Read-Source 'src/server/game/Battlegrounds/BattlegroundQueue.cpp'
$manager = Read-Source 'src/server/game/Battlegrounds/BattlegroundMgr.cpp'
$update = Get-Method $queue 'void BattlegroundQueue::BattlegroundQueueUpdate('
if ($update -notmatch '(?s)SelectRatedTeams\(bracketID, aTeam, hTeam\).*?m_SelectionPools\[TEAM_ALLIANCE\]\.AddGroup\(aTeam, aTeam->Players\.size\(\)\);.*?m_SelectionPools\[TEAM_HORDE\]\.AddGroup\(hTeam, hTeam->Players\.size\(\)\);.*?GenerateRandomMap\(bgTypeId\)') {
    throw 'Rated map selection has not received the two matched rosters'
}
$factory = Get-Method $manager 'Battleground* BattlegroundMgr::CreateNewBattleground('
if ($factory -match 'SelectRandomContainerElement' -or
    $factory -notmatch '(?s)bgTypeId = generatedType;.*?GetBattlegroundData\(bgTypeId\).*?switch \(bgTypeId\)') {
    throw 'Factory does not use the selected map through instance creation'
}
[IO.File]::WriteAllText("$output/select.inc", (Get-Method $queue 'uint16 BattlegroundQueue::GenerateRandomMap('))

$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$build = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" /I"$root/src/server/game/Battlegrounds" "$PSScriptRoot/RatedBattlegroundQueueTest.cpp" /Fe"$output/queue.exe" /Fo"$output/queue.obj"
if errorlevel 1 exit /b 1
"$output/queue.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd", $build)
& "$output/run.cmd"
if ($LASTEXITCODE) { throw 'Battleground map selection regression failed' }
