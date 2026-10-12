param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/legendary-defensive")
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=[IO.File]::ReadAllText("$repo/src/server/scripts/Spells/spell_item.cpp")
$start=$source.IndexOf('class spell_item_norgannons_foresight :')
$begin=$source.IndexOf('uint32 stationaryTime', $start)
$end=$source.IndexOf('void Register()', $begin)
if($start -lt 0 -or $begin -lt 0 -or $end -le $begin){throw 'Norgannon production handlers missing'}
[IO.File]::WriteAllText("$output/Norgannon.inc", $source.Substring($begin,$end-$begin))
$start=$source.IndexOf('class spell_item_prydaz_abs :')
$begin=$source.IndexOf('void CalculateAmount(', $start)
$open=$source.IndexOf('{',$begin);$close=$open+1;$depth=1
while($depth -gt 0 -and $close -lt $source.Length){if($source[$close] -eq '{'){++$depth};if($source[$close] -eq '}'){--$depth};++$close}
if($start -lt 0 -or $begin -lt 0 -or $depth){throw 'Prydaz handler missing'}
[IO.File]::WriteAllText("$output/Prydaz.inc",$source.Substring($begin,$close-$begin))
$paladin=[IO.File]::ReadAllText("$repo/src/server/scripts/Spells/spell_paladin.cpp")
$start=$paladin.IndexOf('class spell_pal_archimondes_hatred_reborn :')
$begin=$paladin.IndexOf('float absorb =', $start);$end=$paladin.IndexOf('void Register()', $begin)
if($start -lt 0 -or $begin -lt 0 -or $end -le $begin){throw 'Archimonde handlers missing'}
[IO.File]::WriteAllText("$output/Archimonde.inc",$paladin.Substring($begin,$end-$begin))
if(!$paladin.Contains('AfterEffectAbsorb += AuraEffectAbsorbFn(spell_pal_archimondes_hatred_reborn_AuraScript::Absorb')){throw 'Archimonde must account for actual absorption after the core limit'}
$unit=[IO.File]::ReadAllText("$repo/src/server/game/Entities/Unit/Unit.cpp")
$begin=$unit.IndexOf('void DamageInfo::ModifyDamage(');$end=$unit.IndexOf('void DamageInfo::ResistDamage(', $begin)
if($begin -lt 0 -or $end -le $begin){throw 'Damage absorb handler missing'}
[IO.File]::WriteAllText("$output/DamageAbsorb.inc",$unit.Substring($begin,$end-$begin))
$begin=$unit.IndexOf('uint32 absorbIgnoringDamage =');$end=$unit.IndexOf('// split damage auras', $begin)
if($begin -lt 0 -or $end -le $begin){throw 'School absorb loop missing'}
[IO.File]::WriteAllText("$output/SchoolAbsorb.inc",$unit.Substring($begin,$end-$begin))
foreach($hook in @('AfterEffectApply += AuraEffectApplyFn(spell_item_norgannons_foresight::OnApply','OnEffectUpdate += AuraEffectUpdateFn(spell_item_norgannons_foresight::OnUpdate','OnEffectPeriodic += AuraEffectPeriodicFn(spell_item_norgannons_foresight::OnTick','AfterEffectRemove += AuraEffectRemoveFn(spell_item_norgannons_foresight::OnRemove','RegisterAuraScript(spell_item_norgannons_foresight);')){
 if(!$source.Contains($hook)){throw "Unregistered handler: $hook"}
}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'MSVC not found'}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/LegendaryDefensiveEffectsTest.cpp" /Fe"$output/LegendaryDefensiveEffectsTest.exe" /Fo"$output/LegendaryDefensiveEffectsTest.obj"
if errorlevel 1 exit /b 1
"$output/LegendaryDefensiveEffectsTest.exe"
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/PrydazAbsorptionTest.cpp" /Fe"$output/PrydazAbsorptionTest.exe" /Fo"$output/PrydazAbsorptionTest.obj"
if errorlevel 1 exit /b 1
"$output/PrydazAbsorptionTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Legendary defensive regression failed'}
