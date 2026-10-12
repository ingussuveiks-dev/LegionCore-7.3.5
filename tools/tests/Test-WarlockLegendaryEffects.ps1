param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/warlock-legendary")
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
Extract $unit 'void TriggerMasterHarvester(' 'MasterHarvester.inc'
$visual=$unit.Substring($unit.IndexOf('void Unit::VisualForPower('))
Extract $visual 'case POWER_SOUL_SHARDS:' 'ShardPayment.inc'
Extract $unit 'case 248113: // The Master Harvester' 'LegacyHarvester.inc'
$warl=[IO.File]::ReadAllText("$repo/src/server/scripts/Spells/spell_warlock.cpp")
$belt=$warl.Substring($warl.IndexOf('class spell_warl_power_cord_of_lethtendris :'))
Extract $belt 'void HandleHit()' 'PowerCord.inc'
$reap=$warl.Substring($warl.IndexOf('class spell_warl_reap_souls :'))
Extract $reap 'SpellCastResult CheckCast()' 'ReapCheck.inc'
Extract $reap 'void HandleAfterCast()' 'ReapAfter.inc'
[IO.File]::WriteAllText("$output/ReapSouls.inc",[IO.File]::ReadAllText("$output/ReapCheck.inc")+"`n"+[IO.File]::ReadAllText("$output/ReapAfter.inc"))
foreach($hook in @('AfterHit += SpellHitFn(spell_warl_power_cord_of_lethtendris::HandleHit)','RegisterSpellScript(spell_warl_power_cord_of_lethtendris);','AfterCast += SpellCastFn(spell_warl_reap_souls_SpellScript::HandleAfterCast)')){
    if(!$warl.Contains($hook)){throw "Missing registration: $hook"}
}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'MSVC not found'}
$commands=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/WarlockLegendaryEffectsTest.cpp" /Fe"$output/WarlockLegendaryEffectsTest.exe" /Fo"$output/WarlockLegendaryEffectsTest.obj"
if errorlevel 1 exit /b 1
"$output/WarlockLegendaryEffectsTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$commands)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Warlock legendary regression failed'}
