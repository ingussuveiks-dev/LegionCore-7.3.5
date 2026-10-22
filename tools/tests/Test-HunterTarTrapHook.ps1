param([string]$BaselineRevision,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory="$PSScriptRoot/../../build-extractors/bin/Release")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hunter-tar-trap-hook"
New-Item -ItemType Directory -Force $out | Out-Null
function Section([string]$s,[string]$start,[string]$end){$a=$s.IndexOf($start);$b=$s.IndexOf($end,$a);if($a -lt 0 -or $b -lt $a){throw "Missing native section $start"};$s.Substring($a,$b-$a)}
$s=Get-Content "$repo/src/server/scripts/Spells/spell_hunter.cpp" -Raw
if($BaselineRevision){$s=(& git -C $repo show "${BaselineRevision}:src/server/scripts/Spells/spell_hunter.cpp") -join "`n";if($LASTEXITCODE){throw 'Cannot read hunter baseline'}}
[IO.File]::WriteAllText("$out/Trap.inc",(Section $s 'class spell_hun_tar_trap_aura' '// Explosive Shot: Detonate!'))
$s=Get-Content "$repo/src/server/game/Spells/SpellScript.cpp" -Raw
[IO.File]::WriteAllText("$out/Target.inc",(Section $s 'Unit* AuraScript::GetTarget() const' 'AuraApplication const* AuraScript::GetTargetApplication() const'))
[IO.File]::WriteAllText("$out/Owner.inc",(Section $s 'Unit* AuraScript::GetUnitOwner() const' 'DynamicObject* AuraScript::GetDynobjOwner() const'))
$s=Get-Content "$repo/src/server/game/Spells/SpellScript.h" -Raw
[IO.File]::WriteAllText("$out/Hooks.inc",((Section $s 'enum SpellScriptState' '// helper class from which SpellScript')+(Section $s 'enum AuraScriptHookType' 'class TC_GAME_API AuraScript')))
$s=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuras.cpp" -Raw
[IO.File]::WriteAllText("$out/Dispatch.inc",(Section $s 'void Aura::CallScriptEffectUpdateHandlers(' 'void Aura::CallScriptAuraUpdateHandlers('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/HunterTarTrapHookTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif %errorlevel% neq 0 exit /b %errorlevel%`n`"$out/test.exe`"`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Hunter Tar Trap native effect-update regression failed'}
if($VerifyReleaseBinary){
 $python=(Get-Command python -ErrorAction SilentlyContinue | Select-Object -First 1).Source
 if(!$python){$python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
 & $python "$PSScriptRoot/verify_hunter_tar_trap_release.py" $ReleaseDirectory
 if($LASTEXITCODE){throw 'Linked hunter Tar Trap callback verification failed'}
}
