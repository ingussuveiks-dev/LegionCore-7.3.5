param([string]$BaselineRevision)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/aura-script-reentry"
New-Item -ItemType Directory -Force -Path $output | Out-Null
function Extract([string]$source,[string]$start,[string]$end) {
    $a=$source.IndexOf($start); $b=$source.IndexOf($end,$a)
    if($a -lt 0 -or $b -lt 0){throw "Production section not found: $start"}
    return $source.Substring($a,$b-$a)
}
$header=Get-Content "$repo/src/server/game/Spells/SpellScript.h" -Raw
$script=Get-Content "$repo/src/server/game/Spells/SpellScript.cpp" -Raw
$aura=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuras.cpp" -Raw
if($BaselineRevision){
    $aura=(& git -C $repo show "${BaselineRevision}:src/server/game/Spells/Auras/SpellAuras.cpp") -join "`n"
    if($LASTEXITCODE){throw 'Cannot read baseline aura dispatchers'}
}
$warlock=Get-Content "$repo/src/server/scripts/Spells/spell_warlock.cpp" -Raw
[IO.File]::WriteAllText("$output/AuraHooks.inc",(Extract $header 'enum AuraScriptHookType' 'class TC_GAME_API AuraScript'))
[IO.File]::WriteAllText("$output/AuraState.inc",(Extract $header '        class ScriptStateStore' '        sf::contention_free_shared_mutex'))
[IO.File]::WriteAllText("$output/AuraLifecycle.inc",(Extract $script 'void AuraScript::_PrepareScriptCall(' 'SpellInfo const* AuraScript::GetSpellInfo()'))
$dispatch=(Extract $aura 'bool Aura::CallScriptCheckAreaTargetHandlers(' 'void Aura::CallScriptDispel(')
$dispatch+=(Extract $aura 'bool Aura::CallScriptCheckProcHandlers(' 'void Aura::SetScriptData(')
$dispatch+=(Extract $aura 'bool Aura::CallScriptEffectProcHandlers(' 'UnitAura::UnitAura(')
[IO.File]::WriteAllText("$output/AuraDispatch.inc",$dispatch)
[IO.File]::WriteAllText("$output/ImmolateProc.inc",(Extract $warlock 'class spell_warl_immolate_proc :' '// Conflagrate -'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$output`" `"$PSScriptRoot/AuraScriptReentryTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$output/test.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Aura script reentry regression failed'}
