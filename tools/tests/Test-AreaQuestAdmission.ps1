param(
    [string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/area-quest-admission",
    [switch]$Baseline
)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $out | Out-Null
$path = 'src/server/game/Entities/Player/Player.cpp'
$source = if ($Baseline) {
    $previous = & git -C $repo show "248cff66a:$path"
    if ($LASTEXITCODE) { throw 'Pinned pre-repair source could not be read' }
    $previous -join [Environment]::NewLine
} else {
    Get-Content -LiteralPath "$repo/$path" -Raw
}
$methods = foreach ($method in 'void Player::PrepareAreaQuest','bool Player::CanAddQuest') {
    $body = [regex]::Match($source, "(?ms)^$method\(.*?^\}").Value
    if (!$body) { throw "Missing $method" }
    $body
}
[IO.File]::WriteAllText("$out/AreaQuestMethods.inc", ($methods -join [Environment]::NewLine))
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$out" "$PSScriptRoot/AreaQuestAdmissionTest.cpp" /Fe"$out/test.exe" /Fo"$out/test.obj"
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Area quest admission regression failed' }
