param([string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/glyph-lifecycle")
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
Extract 'src/server/game/Spells/Spell.cpp' 'case SPELL_EFFECT_APPLY_GLYPH:' 'GlyphCheck.inc'
Extract 'src/server/game/Spells/SpellEffects.cpp' 'void Spell::EffectApplyGlyph(' 'GlyphApply.inc'
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$commands = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/GlyphLifecycleTest.cpp" /Fe"$output/GlyphLifecycleTest.exe" /Fo"$output/GlyphLifecycleTest.obj"
if errorlevel 1 exit /b 1
"$output/GlyphLifecycleTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd", $commands)
& "$output/run.cmd"
if ($LASTEXITCODE -ne 0) { throw "Glyph lifecycle regression failed: $LASTEXITCODE" }
