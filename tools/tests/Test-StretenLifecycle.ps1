param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/streten-lifecycle")
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=[IO.File]::ReadAllText("$repo/src/server/scripts/Spells/spell_warlock.cpp")
$start=$source.IndexOf('class spell_warl_streten_insanity :')
$end=$source.IndexOf('// Seed of Corruption', $start)
if($start -lt 0 -or $end -le $start){throw 'Streten script not found'}
$script=$source.Substring($start,$end-$start)
foreach($entry in @(@('void OnApply(', 'StretenApply.inc'),@('void OnUpdate(', 'StretenUpdate.inc'),@('void OnRemove(', 'StretenRemove.inc'))){
 $begin=$script.IndexOf($entry[0]); if($begin -lt 0){throw "Missing handler $($entry[0])"}
 $open=$script.IndexOf('{',$begin); $close=$open+1; $depth=1
 while($depth -gt 0 -and $close -lt $script.Length){if($script[$close] -eq '{'){++$depth};if($script[$close] -eq '}'){--$depth};++$close}
 if($depth){throw 'Unbalanced handler'}
 [IO.File]::WriteAllText("$output/$($entry[1])",$script.Substring($begin,$close-$begin))
}
foreach($hook in @('AfterEffectApply += AuraEffectApplyFn(spell_warl_streten_insanity_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);','AfterEffectRemove += AuraEffectRemoveFn(spell_warl_streten_insanity_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);')){
 if(!$script.Contains($hook)){throw 'Lifecycle handler not registered'}
}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'MSVC not found'}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/StretenLifecycleTest.cpp" /Fe"$output/StretenLifecycleTest.exe" /Fo"$output/StretenLifecycleTest.obj"
if errorlevel 1 exit /b 1
"$output/StretenLifecycleTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Streten lifecycle regression failed'}
