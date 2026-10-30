param([string]$DataDirectory="$PSScriptRoot/../../build-extractors/runtime-test/talonclaw-boarded-departure-data",[switch]$VerifyNativeCatalog,[switch]$VerifyReleaseBinary,[switch]$VerifyLiveData,[string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/talonclaw-boarded-departure")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo,"$repo/build-extractors/runtime-test/talonclaw-boarded-departure-data","$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 & "$PSScriptRoot/Test-TalonclawBoardedDepartureData.ps1" -VerifyLiveData:$VerifyLiveData
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$i=$s.IndexOf('{',$a)+1;$depth=1;while($depth){if($s[$i] -eq '{'){++$depth};if($s[$i] -eq '}'){--$depth};++$i};$s.Substring($a,$i-$a)}
 $header=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$methods=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
 [IO.File]::WriteAllText("$out/FunctionProcessor.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $header 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$methods.Substring($methods.IndexOf('FunctionProcessor::FunctionProcessor()')))
 [IO.File]::WriteAllText("$out/QuestDelay.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::AddQuestDelayedEvent('))
 [IO.File]::WriteAllText("$out/EnterVehicle.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw) 'void Unit::EnterVehicle(').Replace('Unit::EnterVehicle','Player::EnterVehicle'))
 [IO.File]::WriteAllText("$out/SmartClick.inc",(Block (Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw) 'void SmartAI::OnSpellClick('))
 $smart=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 [IO.File]::WriteAllText("$out/CreditPlayer.inc",(Block $smart 'Player* GetScriptCreditPlayer('))
 $a=$smart.IndexOf('        case SMART_ACTION_CALL_KILLEDMONSTER:');$b=$smart.IndexOf('        case SMART_ACTION_SET_INST_DATA:',$a);[IO.File]::WriteAllText("$out/CreditAction.inc",$smart.Substring($a,$b-$a).Replace('SMART_ACTION_CALL_KILLEDMONSTER','33'))
 $a=$smart.IndexOf('tempLastInvoker->AddDelayedEvent(200,');$b=$smart.IndexOf('});',$a)+3;[IO.File]::WriteAllText("$out/InvokerQueue.inc",$smart.Substring($a,$b-$a))
 Copy-Item -LiteralPath "$repo/src/server/scripts/Legion/ClassHalls/talonclaw_boarded_departure.h" -Destination "$out/TalonclawBoardedDeparture.inc"
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/TalonclawBoardedDepartureTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`" `"$DataDirectory/original.tsv`" `"$DataDirectory/repaired.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Boarded departure native/ASAN regression failed'}
 if($VerifyNativeCatalog){$prior=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TalonclawBoardedDepartureCatalog.cpp -ProbeName TalonclawBoardedDepartureCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
  $native=@(Get-Content "$out/native.tsv")
  foreach($record in @("EFFECT`t46598`t0`t6`t236`t0`t0`t25`t0`t0","EFFECT`t198941`t0`t28`t0`t100967`t0`t46`t0`t61","EFFECT`t198950`t0`t227`t0`t1321`t0`t1`t17`t1384","SUMMON`t61`t1`t2`t2","SEAT`t4535`t16201`t1073741831`t33554432`t262912`t0`t0`t0`t0","USABILITY`t16201`t0`t1")){if($native -notcontains $record){throw "Native boarding assets differ: $record"}}
  'PASS native26972 real forced-use seat16201 (not a free entry/control seat),guardian61,46598 boarding and original198950 teleport/destination selectors'
 }
 if($VerifyReleaseBinary){$prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_talonclaw_boarded_departure_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Selected canonical boarded-departure PE check failed'}}finally{$env:PYTHONPATH=$prior}}

}
