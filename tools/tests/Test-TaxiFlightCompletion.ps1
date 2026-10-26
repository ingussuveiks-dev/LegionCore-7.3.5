param([switch]$Baseline,[switch]$AddressSanitizer,[switch]$VerifyNativeCatalog,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory="$PSScriptRoot/../../build-extractors/bin/Release")
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify repaired Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/taxi-flight-completion"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $path='src/server/game/Movement/MovementGenerators/FlightPathMovementGenerator.cpp'
 $source=Get-Content "$repo/$path" -Raw -Encoding UTF8
 $pinned=(& git -C $repo show "5ec963d5:$path") -join "`n"
 if($LASTEXITCODE){throw 'Pinned flight baseline unavailable'}
 $marker='void FlightPathMovementGenerator::DoFinalize('
 if(($source.Replace((Block $source $marker),(Block $pinned $marker)) -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Unreviewed flight change outside finalization'}
 $selected=if($Baseline){$pinned}else{$source}
 [IO.File]::WriteAllText("$out/FlightMethods.inc",((@('bool IsNodeIncludedInShortenedPath(','void FlightPathMovementGenerator::LoadPath(','bool FlightPathMovementGenerator::DoUpdate(','void FlightPathMovementGenerator::SetCurrentNodeAfterTeleport(')|ForEach-Object {Block $source $_}) -join "`n")+(Block $selected $marker))
 $header=Get-Content "$repo/src/server/game/Entities/Player/PlayerTaxi.h" -Raw -Encoding UTF8
 [IO.File]::WriteAllText("$out/PlayerTaxiClass.inc",(Block $header 'class PlayerTaxi')+';')
 $codec=Get-Content "$repo/src/server/game/Entities/Player/PlayerTaxi.cpp" -Raw -Encoding UTF8
 $markers=@('PlayerTaxi::PlayerTaxi()','bool PlayerTaxi::LoadTaxiDestinationsFromString(','std::string PlayerTaxi::SaveTaxiDestinationsToString(','void PlayerTaxi::ClearTaxiDestinations(','void PlayerTaxi::AddTaxiDestination(','void PlayerTaxi::SetTaxiDestination(','uint32 PlayerTaxi::GetTaxiSource(','uint32 PlayerTaxi::GetTaxiDestination(','uint32 PlayerTaxi::GetCurrentTaxiPath(','uint32 PlayerTaxi::NextTaxiDestination(','std::deque<uint32> const& PlayerTaxi::GetPath(','bool PlayerTaxi::empty(')
 [IO.File]::WriteAllText("$out/PlayerTaxiMethods.inc",(($markers|ForEach-Object {Block $codec $_}) -join "`n"))
 $utility=Get-Content "$repo/src/common/Utilities/Util.h" -Raw -Encoding UTF8
 [IO.File]::WriteAllText("$out/Tokenizer.inc",(Block $utility 'class TC_COMMON_API Tokenizer').Replace('TC_COMMON_API ','')+';'+(Block (Get-Content "$repo/src/common/Utilities/Util.cpp" -Raw -Encoding UTF8) 'Tokenizer::Tokenizer(')+(Block (Get-Content "$repo/src/common/Common.h" -Raw -Encoding UTF8) 'inline unsigned long atoul('))
 [IO.File]::WriteAllText("$out/PlayerCleanup.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw -Encoding UTF8) 'void Player::CleanupAfterTaxiFlight('))
 [IO.File]::WriteAllText("$out/ClientAck.inc",(Block (Get-Content "$repo/src/server/game/Handlers/MovementHandler.cpp" -Raw -Encoding UTF8) 'void WorldSession::HandleMoveSplineDone('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $definition=if($Baseline){'/DBASELINE'}else{''};if($AddressSanitizer){$definition+=' /fsanitize=address /Zi'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/TaxiFlightCompletionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native taxi completion regression failed'}
 if($VerifyNativeCatalog){
  $previous=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TrueshotTaxiCatalog.cpp -ProbeName TrueshotTaxiCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Trueshot native catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
  $rows=Get-Content "$out/native.tsv" -Encoding UTF8
  foreach($row in @("NODE`t1760`tTrueshot Lodge, Highmountain`t4634.29`t5339.42`t849.231`t11`t38053`t69067`t69067","NODE`t1848`tTrueshot Lodge`t4632.03`t5320.96`t852.012`t11`t43356`t98937`t98937","PATH`t5348`t1719`t1760`t11000`t13","PATH`t5716`t1847`t1848`t0`t12")){if($rows -notcontains $row){throw 'Distinct Trueshot taxi nodes/routes changed'}}
  'PASS native26972/read-only hotfix separate1760 Odan paid hub and1848 Great Eagle free-route hub; both flightmaster icons have native route evidence'
 }
 if($VerifyReleaseBinary){
  $previous=$env:PYTHONPATH;$previousErrors=$ErrorActionPreference
  try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";$ErrorActionPreference='Continue';& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_taxi_flight_completion_release.py" $ReleaseDirectory *> "$out/pe-check.log";$code=$LASTEXITCODE;$ErrorActionPreference=$previousErrors;Get-Content "$out/pe-check.log" -Encoding UTF8;if($code){throw 'Linked taxi completion repair missing'}}finally{$env:PYTHONPATH=$previous;$ErrorActionPreference=$previousErrors}
 }
}
