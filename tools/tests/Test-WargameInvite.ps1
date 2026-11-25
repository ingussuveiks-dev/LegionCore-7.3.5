param([Parameter(Mandatory)][string]$OutputDirectory, [switch]$Baseline)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot/../..").Path
$sourcePath = 'src/server/game/Battlegrounds/BattlegroundMgr.cpp'
$source = if ($Baseline) { (& git -C $root show "484ea0d06:$sourcePath") -join "`n" } else {
    Get-Content -LiteralPath "$root/$sourcePath" -Raw
}
$body = [regex]::Match($source, '(?ms)^void BattlegroundMgr::InitWargame\(.*?^\}').Value
if (!$body) { throw 'Missing production Wargame invitation handler' }
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $output -Force | Out-Null
[IO.File]::WriteAllText("$output/invite.inc", $body)

$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$build = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" /I"$root/src/server/game/Battlegrounds" "$PSScriptRoot/WargameInviteTest.cpp" /Fe"$output/invite.exe" /Fo"$output/invite.obj"
if errorlevel 1 exit /b 1
"$output/invite.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd", $build)
& "$output/run.cmd"
if ($LASTEXITCODE) { throw "Wargame invitation regression failed with exit $LASTEXITCODE" }
