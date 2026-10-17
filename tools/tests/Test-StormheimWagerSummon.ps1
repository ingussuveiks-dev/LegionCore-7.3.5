$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-StormheimWagerData.ps1"
$out="$repo/build-extractors/runtime-test/stormheim-wager"
$effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
$a=$effects.IndexOf('void Spell::EffectSendEvent(');$b=$effects.IndexOf('void Spell::EffectPowerBurn(',$a)
[IO.File]::WriteAllText("$out/SendEvent.inc",$effects.Substring($a,$b-$a))
$maps=Get-Content "$repo/src/server/game/Scripting/MapScripts.cpp" -Raw
$a=$maps.IndexOf('            case SCRIPT_COMMAND_TEMP_SUMMON_CREATURE:');$b=$maps.IndexOf('            case SCRIPT_COMMAND_OPEN_DOOR:',$a)
[IO.File]::WriteAllText("$out/EventSummon.inc",$maps.Substring($a,$b-$a))
$summon=Get-Content "$repo/src/server/game/Entities/Creature/TemporarySummon.cpp" -Raw
$a=$summon.IndexOf('void TempSummon::Update(');$b=$summon.IndexOf('void TempSummon::InitStats(',$a)
[IO.File]::WriteAllText("$out/SummonUpdate.inc",$summon.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimWagerSummonTest.cpp`" /Fe`"$out/WagerSummon.exe`" /Fo`"$out/WagerSummon.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/WagerSummon.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run-summon.cmd",$cmd);& "$out/run-summon.cmd"
if($LASTEXITCODE){throw 'Wager native summon regression failed'}
