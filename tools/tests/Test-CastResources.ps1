param([string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/cast-resources")
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
function Extract($file, $marker, $destination) {
    $source = [IO.File]::ReadAllText("$repo/$file")
    $start = $source.IndexOf($marker)
    if ($start -lt 0) { throw "Missing production code: $marker" }
    $open = $source.IndexOf('{', $start)
    $depth = 1
    $end = $open + 1
    while ($depth -gt 0 -and $end -lt $source.Length) {
        if ($source[$end] -eq '{') { ++$depth }
        if ($source[$end] -eq '}') { --$depth }
        ++$end
    }
    if ($depth -ne 0) { throw "Unbalanced production code: $marker" }
    [IO.File]::WriteAllText("$output/$destination", $source.Substring($start, $end - $start))
}
$player = 'src/server/game/Entities/Player/Player.cpp'
foreach ($name in @('TakeSpellCharge','UpdateSpellCharges','ModSpellCharge','ModSpellChargeCooldown','SendSpellChargeData')) {
    Extract $player "void Player::$name(" "$name.inc"
}
Extract $player 'void Player::_LoadSpellCharges(' 'LoadCharges.inc'
Extract $player 'void Player::_SaveSpellCharges(' 'SaveCharges.inc'
Extract $player 'uint32 Player::GetSpellCategoryChargesTimer(' 'GetChargeTimer.inc'
Extract $player 'uint32 Player::GetChargesCooldown(' 'GetChargeCooldown.inc'
Extract 'src/server/game/Spells/Spell.cpp' 'SpellCastResult Spell::CheckPower(' 'CheckPower.inc'
Extract 'src/server/game/Spells/Spell.cpp' 'void Spell::TakeReagents(' 'TakeReagents.inc'
Extract 'src/server/game/Spells/Spell.cpp' 'void Spell::TakePower(' 'TakePower.inc'
$stores = [IO.File]::ReadAllText("$repo/src/server/game/DataStores/DB2Stores.cpp")
$pruneStart = $stores.IndexOf('    for (auto& spec : _specializationSpellsBySpec)')
$pruneEnd = $stores.IndexOf('    TC_LOG_INFO', $pruneStart)
if ($pruneStart -lt 0 -or $pruneEnd -le $pruneStart) { throw 'Specialization tombstone cleanup missing' }
[IO.File]::WriteAllText("$output/PruneLinks.inc", $stores.Substring($pruneStart, $pruneEnd - $pruneStart))
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$commands = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/CastResourceTest.cpp" /Fe"$output/CastResourceTest.exe" /Fo"$output/CastResourceTest.obj"
if errorlevel 1 exit /b 1
"$output/CastResourceTest.exe"
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 /I"$output" "$PSScriptRoot/ReagentConsumptionTest.cpp" /Fe"$output/ReagentConsumptionTest.exe" /Fo"$output/ReagentConsumptionTest.obj"
if errorlevel 1 exit /b 1
"$output/ReagentConsumptionTest.exe"
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/PowerConsumptionTest.cpp" /Fe"$output/PowerConsumptionTest.exe" /Fo"$output/PowerConsumptionTest.obj"
if errorlevel 1 exit /b 1
"$output/PowerConsumptionTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd", $commands)
& "$output/run.cmd"
if ($LASTEXITCODE -ne 0) { throw "Cast resource regression failed: $LASTEXITCODE" }
