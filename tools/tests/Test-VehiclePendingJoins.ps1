param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned pending-event baseline cannot verify Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/vehicle-pending-joins"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/game/Entities/Vehicle/Vehicle.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "f784d4fb:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pending-event source missing'}}
foreach($pair in @(@('Remove','void Vehicle::RemovePendingEvent('),@('Add','void Vehicle::AddPendingEvent('),@('Seat','void Vehicle::RemovePendingEventsForSeat('),@('Passenger','void Vehicle::RemovePendingEventsForPassenger('),@('All','void Vehicle::RemoveAllPassengers('),@('Construct','VehicleJoinEvent::VehicleJoinEvent('),@('Destroy','VehicleJoinEvent::~VehicleJoinEvent('),@('Abort','void VehicleJoinEvent::Abort('))){
 [IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $source ("`n"+$pair[1])))
}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$out`" /I`"$repo/src/common`" /I`"$repo/src/common/Utilities`" `"$PSScriptRoot/VehiclePendingJoinsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native vehicle pending-event regression failed'}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_vehicle_pending_joins_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed pending-event guard failed'}}finally{$env:PYTHONPATH=$previous}
}
