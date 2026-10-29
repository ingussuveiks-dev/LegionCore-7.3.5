param([string]$DataDirectory="$PSScriptRoot/../../build-extractors/runtime-test/meryl-wave-data",[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/meryl-wave"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories "$repo/build-extractors/runtime-test/legion-legendary-catalog" -Test {
 function Section([string]$text,[string]$start,[string]$end){$i=$text.IndexOf($start);$j=$text.IndexOf($end,[Math]::Max(0,$i));if($i -lt 0 -or $j -le $i){throw "Missing native $start"};$text.Substring($i,$j-$i)}
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 [IO.File]::WriteAllText("$out/MerylSetScript9.inc",(Section $s 'void SmartScript::SetScript9(' 'Unit* SmartScript::GetLastInvoker('))
 $summon=Section $s '        case SMART_ACTION_SUMMON_CREATURE:' '        case SMART_ACTION_SUMMON_GO:'
 $position=Section $summon '            if (e.GetTargetType() == SMART_TARGET_POSITION)' '            break;'
 [IO.File]::WriteAllText("$out/MerylPositionSummon.inc",$position)
 $cs=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
 [IO.File]::WriteAllText("$out/MerylScenarioCondition.inc",(Section $cs '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:'))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $data=[IO.Path]::GetFullPath($DataDirectory)
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/MerylWaveTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$data/original.tsv`" `"$data/repaired.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Meryl wave handoff regression failed'}
 if($VerifyNativeCatalog){$old=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile MerylWaveCatalog.cpp -ProbeName MerylWaveCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$old}
  $native=Get-Content "$out/native.tsv";foreach($line in @("SCENARIO_EVENT`t51384`t1122`t2`t48463`t48464","SCENARIO_EVENT`t51386`t1122`t2`t48463`t48465","SCENARIO_EVENT`t51391`t1122`t4`t48469`t48470","EFFECT`t214729`t0`t0`t219`t0`t2406`t0`t0`t18`t0`t0")){if($native -notcontains $line){throw "Actual26972 Meryl scenario differs: $line"}}
  if(!@($native|Where-Object {$_ -like "STEP`t1122`t3`t*"}).Count){throw 'Native1122 has no stage3'}
  'PASS matching26972 Meryl1122 stages2/3/4 and retained native ambush conversation2406; wave gate remains actual actor1122/stage3'
 }
}
