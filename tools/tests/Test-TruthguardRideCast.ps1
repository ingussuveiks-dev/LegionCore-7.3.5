param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned baseline cannot verify Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/truthguard-ride-cast"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/shields_rest.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "93e6c64c:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned ride baseline missing'}}
$actor=Block $source 'class npc_sr_argent_hippogryph'
foreach($pair in @(@('Spawn','void IsSummonedBy('),@('Rider','Player* GetRider('))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $actor $pair[1]))}
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw
[IO.File]::WriteAllText("$out/ToTemp.inc",(Block $unit 'TempSummon* ToTempSummon('))
[IO.File]::WriteAllText("$out/ToTempConst.inc",(Block $unit 'TempSummon const* ToTempSummon('))
[IO.File]::WriteAllText("$out/VehicleBase.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw) 'Unit* Unit::GetVehicleBase('))
$temp=Get-Content "$repo/src/server/game/Entities/Creature/TemporarySummon.cpp" -Raw
foreach($pair in @(@('Summoner','Unit* TempSummon::GetSummoner('),@('InitSummon','void TempSummon::InitSummon('),@('Delayed','bool ForcedUnsummonDelayEvent::Execute('))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $temp $pair[1]))}
[IO.File]::WriteAllText("$out/EmptySeat.inc",(Block (Get-Content "$repo/src/server/game/Entities/Vehicle/Vehicle.cpp" -Raw) "`nbool Vehicle::HasEmptySeat("))
$despawn=Block (Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw) 'void Creature::DespawnOrUnsummon('
$despawn=$despawn.Replace(' /*= 0*/','')
[IO.File]::WriteAllText("$out/Despawn.inc",$despawn)
[IO.File]::WriteAllText("$out/CastResults.inc",(Block (Get-Content "$repo/src/server/game/Miscellaneous/SharedDefines.h" -Raw) 'enum SpellCastResult')+';')
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$out`" `"$PSScriptRoot/TruthguardRideCastTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Truthguard ride host failed'}
if($VerifyReleaseBinary){
 & "$PSScriptRoot/Test-TruthguardRider.ps1" -VerifyReleaseBinary *> "$out/rider.log";if(!$?){throw 'Native rider/vehicle metadata guard failed'}
 $native=Get-Content "$repo/build-extractors/runtime-test/truthguard-rider/native.tsv"
 foreach($expected in @("TRAVEL_SUMMON`t209884`t2061`t1`t0`t2048`t0","TRAVEL_SUMMON`t210358`t2061`t1`t0`t2048`t0","TRAVEL_DURATION`t209884`t3`t60000`t60000","TRAVEL_DURATION`t210358`t3`t60000`t60000")){if($native -notcontains $expected){throw "Ride lifetime/category changed: $expected"}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_truthguard_ride_cast_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed ride cast guard failed'}}finally{$env:PYTHONPATH=$previous}
}
