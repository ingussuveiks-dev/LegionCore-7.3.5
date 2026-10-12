param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/warlock-pet-healing")
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
function Extract($source,$marker,$name) {
    $start=$source.IndexOf($marker)
    if($start -lt 0){throw "Missing production handler: $marker"}
    $end=$source.IndexOf('{',$start)+1;$depth=1
    while($depth -and $end -lt $source.Length){if($source[$end] -eq '{'){++$depth};if($source[$end] -eq '}'){--$depth};++$end}
    if($depth){throw 'Unbalanced handler'}
    [IO.File]::WriteAllText("$output/$name",$source.Substring($start,$end-$start))
}
$unit=[IO.File]::ReadAllText("$repo/src/server/game/Entities/Unit/Unit.cpp")
Extract $unit 'float Unit::AddSoulLeechAbsorb(' 'SoulLeechAbsorb.inc'
Extract $unit 'void Unit::HealFromSoulLeech(' 'SoulLinkHeal.inc'
Extract $unit 'case 108370: // Soul Leech' 'SoulLeechProc.inc'
$creature=[IO.File]::ReadAllText("$repo/src/server/game/Entities/Creature/Creature.cpp")
Extract $creature 'void Creature::RegenerateHealth()' 'PetRegen.inc'
$warl=[IO.File]::ReadAllText("$repo/src/server/scripts/Spells/spell_warlock.cpp")
$funnel=$warl.Substring($warl.IndexOf('class spell_warl_health_funnel :'))
Extract $funnel 'void HandleDamage(' 'FunnelDamage.inc'
Extract $funnel 'void HandleHeal(' 'FunnelHeal.inc'
Extract $funnel 'void HandleCast()' 'FunnelStop.inc'
$skin=$warl.Substring($warl.IndexOf('class spell_warl_demon_skin :'))
Extract $skin 'void OnTick(' 'DemonSkin.inc'
if($warl.Contains('new spell_warl_soul_link();')){throw 'Obsolete damage-split healing is still registered'}
foreach($hook in @('OnEffectHitTarget += SpellEffectFn(spell_warl_health_funnel_SpellScript::HandleHeal, EFFECT_1, SPELL_EFFECT_HEAL)','OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_demon_skin_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY)')){
    if(!$warl.Contains($hook)){throw "Missing registration: $hook"}
}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'MSVC not found'}
$commands=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/WarlockPetHealingTest.cpp" /Fe"$output/WarlockPetHealingTest.exe" /Fo"$output/WarlockPetHealingTest.obj"
if errorlevel 1 exit /b 1
"$output/WarlockPetHealingTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$commands)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Warlock pet healing regression failed'}
