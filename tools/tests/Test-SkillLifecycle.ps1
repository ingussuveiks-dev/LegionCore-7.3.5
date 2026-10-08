param([string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/skill-lifecycle")
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
function Extract($file, $marker, $destination, $bodyOnly = $false) {
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
    if ($bodyOnly) { $start = $open }
    [IO.File]::WriteAllText("$output/$destination", $source.Substring($start, $end - $start))
}
$player = 'src/server/game/Entities/Player/Player.cpp'
Extract $player 'void Player::SetSkill(' 'SkillSet.inc'
Extract $player 'uint16 Player::GetSkillValue(' 'SkillValue.inc'
Extract $player 'void Player::ModifySkillBonus(' 'SkillBonus.inc'
Extract $player 'void Player::UpdateSkillEnchantments(' 'SkillEnchantments.inc'
Extract $player 'int16 Player::GetSkillPermBonusValue(' 'SkillPermBonus.inc'
Extract $player 'int16 Player::GetSkillTempBonusValue(' 'SkillTempBonus.inc'
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$commands = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/SkillLifecycleTest.cpp" /Fe"$output/SkillLifecycleTest.exe" /Fo"$output/SkillLifecycleTest.obj"
if errorlevel 1 exit /b 1
"$output/SkillLifecycleTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd", $commands)
& "$output/run.cmd"
if ($LASTEXITCODE -ne 0) { throw "Skill lifecycle regression failed: $LASTEXITCODE" }
