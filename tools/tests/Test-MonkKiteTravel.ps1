param([switch]$AddressSanitizer,[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/monk-kite-travel"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){if($s[$b] -eq '{'){++$depth};if($s[$b] -eq '}'){--$depth};++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($repo,$out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 Copy-Item "$repo/src/server/scripts/Legion/ClassHalls/monk_kite_travel.h" "$out/MonkTravel.inc"
 [IO.File]::WriteAllText("$out/NativeClick.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw) 'bool Unit::HandleSpellClick('))
 [IO.File]::WriteAllText("$out/NativeOnClick.inc",(Block (Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw) 'void SmartAI::OnSpellClick('))
 [IO.File]::WriteAllText("$out/NativePersonal.inc",(Block (Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw) 'void Spell::EffectKillCreditPersonal('))
 $smart=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 $nativeGroup='targetPlayer->RewardPlayerAndGroupAtEvent(e.action.killedMonster.creature, targetPlayer);'
 if(!$smart.Contains($nativeGroup)){throw 'Native group action33 changed'}
 [IO.File]::WriteAllText("$out/NativeGroupCredit.inc",$nativeGroup)
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/MonkKiteTravelTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Monk kite travel regression failed'}
 if($VerifyNativeCatalog){
  $old=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile MonkTravelCatalog.cpp -ProbeName MonkTravelCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$old}
  $rows=Get-Content "$out/native.tsv"
  foreach($line in @("EFFECT`t202475`t0`t0`t140`t0`t0`t202474`t1`t0`t0","EFFECT`t202475`t0`t2`t90`t0`t102531`t0`t1`t0`t0","EFFECT`t202481`t0`t0`t252`t0`t1274`t0`t1`t17`t0")){if($rows -notcontains $line){throw "Native Monk kite contract differs: $line"}}
 }
}
