param([switch]$Baseline,[switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/boost-heroic-leap$(if($Baseline){'-baseline'})"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$marker){$a=$source.IndexOf($marker);if($a -lt 0){throw "Missing $marker"};$b=$source.IndexOf('{',$a)+1;$depth=1;while($depth){if($source[$b] -eq '{'){$depth++};if($source[$b] -eq '}'){$depth--};$b++};$source.Substring($a,$b-$a)}
$source=Get-Content "$repo/src/server/scripts/Spells/spell_warrior.cpp" -Raw
if($Baseline){$source=(& git -C $repo show 'a9da566d:src/server/scripts/Spells/spell_warrior.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned leap baseline missing'}}
$source=$source.Substring($source.IndexOf('class spell_warr_heroic_leap :'))
[IO.File]::WriteAllText("$out/Check.inc",(Block $source 'SpellCastResult CheckElevation()'))
[IO.File]::WriteAllText("$out/Cast.inc",(Block $source 'void HandleDummy('))
foreach($name in @('Transport.h','GameObjectModel.h','MovementTypedefs.h')){[IO.File]::WriteAllText("$out/$name",'// Mock world boundary; real native collision runs in Test-BoostShipFloor.ps1.')}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$flags=if($Baseline){'/DLEAP_BASELINE'}else{''};if($AddressSanitizer){$flags+=' /fsanitize=address /Zi'}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $flags /I`"$out`" /I`"$repo/src/server/game/Movement`" `"$PSScriptRoot/BoostHeroicLeapTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Heroic Leap regression failed'}
