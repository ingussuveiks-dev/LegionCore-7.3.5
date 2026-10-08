param([string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/character-spell-chains")
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source = [IO.File]::ReadAllText("$repo/src/server/scripts/Spells/spell_dk.cpp")
$classStart = $source.IndexOf('class spell_dk_death_coil :')
$start = $source.IndexOf('void HandleDummy(', $classStart)
if ($classStart -lt 0 -or $start -lt 0) { throw 'Cannot locate Death Coil production handler' }
$open = $source.IndexOf('{', $start)
$depth = 1
$end = $open + 1
while ($depth -gt 0 -and $end -lt $source.Length) {
    if ($source[$end] -eq '{') { ++$depth }
    if ($source[$end] -eq '}') { --$depth }
    ++$end
}
if ($depth -ne 0) { throw 'Unbalanced production handler' }
[IO.File]::WriteAllText("$output/DeathCoilDummy.inc", $source.Substring($start, $end - $start))
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$commands = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/CharacterSpellChainsTest.cpp" /Fe"$output/CharacterSpellChainsTest.exe" /Fo"$output/CharacterSpellChainsTest.obj"
if errorlevel 1 exit /b 1
"$output/CharacterSpellChainsTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd", $commands)
& "$output/run.cmd"
if ($LASTEXITCODE -ne 0) { throw "Character spell chain regression failed: $LASTEXITCODE" }
