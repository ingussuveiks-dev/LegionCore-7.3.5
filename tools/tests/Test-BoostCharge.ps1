param([switch]$Baseline,[switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/boost-charge$(if($Baseline){'-baseline'})"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$marker){$a=$source.IndexOf($marker);if($a -lt 0){throw "Missing $marker"};$b=$source.IndexOf('{',$a)+1;$depth=1;while($depth){if($source[$b] -eq '{'){$depth++};if($source[$b] -eq '}'){$depth--};$b++};$source.Substring($a,$b-$a)}
$source=Get-Content "$repo/src/server/scripts/Spells/spell_warrior.cpp" -Raw
if($Baseline){$source=(& git -C $repo show '7ad477f7:src/server/scripts/Spells/spell_warrior.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned charge baseline missing'}}
$source=$source.Substring($source.IndexOf('class spell_warr_charge_check_cast :'))
[IO.File]::WriteAllText("$out/Check.inc",(Block $source 'SpellCastResult CheckCast()'))
[IO.File]::WriteAllText("$out/Target.inc",(Block (Get-Content "$repo/src/server/game/Movement/PathGenerator.cpp" -Raw) 'bool PathGenerator::CalculatePathToTarget('))
$core=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
$a=$core.IndexOf('            case SPELL_EFFECT_CHARGE:')
$b=$core.IndexOf('            case SPELL_EFFECT_JUMP_DEST:', $a)
if(!$core.Substring($a,$b-$a).Contains('CalculatePathToTarget(target)') -or !$core.Substring($a,$b-$a).Contains('ShortenPathUntilDist')){throw 'Real effect admission must use the target path and retain native contact shortening'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$flags=if($Baseline){'/DBOOST_CHARGE_BASELINE'}else{''};if($AddressSanitizer){$flags+=' /fsanitize=address /Zi'}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $flags /I`"$out`" `"$PSScriptRoot/BoostChargeTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Charge admission regression failed'}
