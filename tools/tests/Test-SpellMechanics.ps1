param([string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/spell-mechanics")
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
Extract $player 'void Player::TogglePvpTalents(' 'TogglePvp.inc'
Extract $player 'void Player::LearnSpecializationSpells(' 'LearnSpec.inc'
Extract $player 'void Player::RemoveSpecializationSpells(' 'RemoveSpec.inc'
Extract $player 'void Player::AddOverrideSpell(' 'AddOverride.inc'
Extract $player 'void Player::RemoveOverrideSpell(' 'RemoveOverride.inc'
Extract $player 'bool Player::HasItemFitToSpellRequirements(' 'EquippedRequirements.inc'
Extract 'src/server/game/Entities/Item/Item.cpp' 'bool Item::IsFitToSpellRequirements(' 'ItemRequirements.inc'
Extract 'src/server/game/Spells/Auras/SpellAuras.cpp' 'bool Aura::ModCharges(' 'ModCharges.inc'
Extract 'src/server/game/Spells/Auras/SpellAuras.cpp' 'bool Aura::ModStackAmount(' 'ModStacks.inc'
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$commands = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/SpellMechanicsTest.cpp" /Fe"$output/SpellMechanicsTest.exe" /Fo"$output/SpellMechanicsTest.obj"
if errorlevel 1 exit /b 1
"$output/SpellMechanicsTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd", $commands)
& "$output/run.cmd"
if ($LASTEXITCODE -ne 0) { throw "Spell mechanics regression failed: $LASTEXITCODE" }
