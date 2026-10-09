param([string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/item-effect-lifecycle")
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
function Extract($file, $marker, $name) {
    $source = [IO.File]::ReadAllText("$repo/$file")
    $start = $source.IndexOf($marker)
    if ($start -lt 0) { throw "Missing production function $marker" }
    $end = $source.IndexOf('{', $start) + 1
    $depth = 1
    while ($depth -and $end -lt $source.Length) {
        if ($source[$end] -eq '{') { ++$depth }
        if ($source[$end] -eq '}') { --$depth }
        ++$end
    }
    if ($depth) { throw 'Unbalanced production function' }
    [IO.File]::WriteAllText("$output/$name", $source.Substring($start, $end - $start))
}
$item = 'src/server/game/Entities/Item/Item.cpp'
$player = 'src/server/game/Entities/Player/Player.cpp'
Extract $item 'void AddItemsSetItem(' 'AddSet.inc'
Extract $item 'void RemoveItemsSetItem(' 'RemoveSet.inc'
Extract $item 'void UpdateItemSetSkill(' 'SetSkillBonus.inc'
Extract $player 'void Player::ApplyItemEquipSpell(' 'ItemEquip.inc'
Extract $player 'void Player::UpdateEquipSpellsAtFormChange(' 'RefreshSet.inc'
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$commands = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/ItemEffectLifecycleTest.cpp" /Fe"$output/ItemEffectLifecycleTest.exe" /Fo"$output/ItemEffectLifecycleTest.obj"
if errorlevel 1 exit /b 1
"$output/ItemEffectLifecycleTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd", $commands)
& "$output/run.cmd"
if ($LASTEXITCODE) { throw 'Item effect lifecycle regression failed' }
