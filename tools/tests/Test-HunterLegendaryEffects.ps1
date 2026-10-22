param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/hunter-legendary")
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force $output | Out-Null
function Extract-Function([string]$File,[string]$Signature,[string]$Destination) {
 $source=[IO.File]::ReadAllText("$repo/$File")
 $begin=$source.IndexOf($Signature)
 if($begin -lt 0){throw "Missing production function $Signature"}
 $open=$source.IndexOf('{',$begin);$end=$open+1;$depth=1
 while($depth -gt 0 -and $end -lt $source.Length){if($source[$end] -eq '{'){++$depth};if($source[$end] -eq '}'){--$depth};++$end}
 if($depth){throw "Unbalanced function $Signature"}
 [IO.File]::WriteAllText("$output/$Destination",$source.Substring($begin,$end-$begin))
}
Extract-Function 'src/server/game/Spells/Auras/SpellAuraEffects.cpp' 'void AuraEffect::HandleEnableExtraTalent(' 'ExtraTalent.inc'
Extract-Function 'src/server/game/Spells/Auras/SpellAuraEffects.cpp' 'void AuraEffect::HandleOverridePetSpecs(' 'OverridePet.inc'
Extract-Function 'src/server/game/Entities/Pet/Pet.cpp' 'void Pet::CheckSpecialization()' 'PetSpec.inc'
$source=[IO.File]::ReadAllText("$repo/src/server/game/Entities/Unit/Unit.cpp")
$begin=$source.IndexOf('case SPELL_TRIGGER_MODIFY_COOLDOWN: //23')
$end=$source.IndexOf('case SPELL_TRIGGER_ADD_DURATION_OR_CAST_DURATION:', $begin)
if($begin -lt 0 -or $end -le $begin){throw 'Missing native cooldown rule dispatcher'}
[IO.File]::WriteAllText("$output/Cooldown.inc",$source.Substring($begin,$end-$begin))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$output`" `"$PSScriptRoot/HunterLegendaryEffectsTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`nif errorlevel 1 exit /b 1`n`"$output/test.exe`"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Hunter legendary production-handler regression failed'}
