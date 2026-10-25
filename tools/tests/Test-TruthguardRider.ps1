param([switch]$Baseline,[switch]$VerifyReleaseBinary,[switch]$EndpointTest,[switch]$EndpointBaseline)
$ErrorActionPreference='Stop'
if(($Baseline -or $EndpointBaseline) -and $VerifyReleaseBinary){throw 'Pinned baseline cannot verify Release'}
if(($Baseline -and $EndpointTest) -or ($EndpointBaseline -and !$EndpointTest)){throw 'Invalid rider baseline selection'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/truthguard-rider"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/shields_rest.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "be077aa6:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned rider baseline missing'}}
if($EndpointBaseline){$source=(& git -C $repo show "618afd7c:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned endpoint baseline missing'}}
$actor=Block $source 'class npc_sr_argent_hippogryph'
foreach($pair in @(@('Boarded','void PassengerBoarded('),@('Arrived','void MovementInform('),@('Update','void UpdateAI('),@('Reset','void Reset('))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $actor $pair[1]))}
[IO.File]::WriteAllText("$out/State.inc",[regex]::Match($actor,'bool arrivalAccepted = false;\s*bool arrivalInProgress = false;\s*uint32 arrivalRetryTimer = 0;').Value)
[IO.File]::WriteAllText("$out/CastResults.inc",(Block (Get-Content "$repo/src/server/game/Miscellaneous/SharedDefines.h" -Raw) 'enum SpellCastResult')+';')
[IO.File]::WriteAllText("$out/Rider.inc",$(if($Baseline){''}else{Block $actor 'Player* GetRider('}))
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
[IO.File]::WriteAllText("$out/AnyOwner.inc",(Block $unit 'Unit* Unit::GetAnyOwner('))
[IO.File]::WriteAllText("$out/VehicleBase.inc",(Block $unit 'Unit* Unit::GetVehicleBase('))
[IO.File]::WriteAllText("$out/ToTemp.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'TempSummon* ToTempSummon('))
[IO.File]::WriteAllText("$out/ToTempConst.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'TempSummon const* ToTempSummon('))
[IO.File]::WriteAllText("$out/Summoner.inc",(Block (Get-Content "$repo/src/server/game/Entities/Creature/TemporarySummon.cpp" -Raw) 'Unit* TempSummon::GetSummoner('))
[IO.File]::WriteAllText("$out/Motion.inc",(Block (Get-Content "$repo/src/server/game/Movement/MotionMaster.h" -Raw) 'enum MovementGeneratorType')+';')
$join=Block (Get-Content "$repo/src/server/game/Entities/Vehicle/Vehicle.cpp" -Raw) 'bool VehicleJoinEvent::Execute('
$assignment=$join.IndexOf('Passenger->m_vehicle = vehicle;')
$seat=$join.IndexOf('Seat->second.Passenger.Guid = Passenger->GetGUID();')
$callback=$join.IndexOf('AI()->PassengerBoarded(Passenger, Seat->first, true)')
if($assignment -lt 0 -or $seat -le $assignment -or $callback -le $seat){throw 'Vehicle callback precedes actual pointer/seat boarding'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
if($EndpointBaseline){$mode='/DENDPOINT_BASELINE'}
$testFile=if($EndpointTest){'TruthguardEndpointTest.cpp'}else{'TruthguardRiderTest.cpp'}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$out`" `"$PSScriptRoot/$testFile`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Truthguard rider host failed'}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$live=@("SET SESSION TRANSACTION READ ONLY; SELECT entry,VehicleId,npcflag,AIName,ScriptName FROM creature_template WHERE entry IN(105886,106205) ORDER BY entry;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Rider template read failed'}}finally{$env:MYSQL_PWD=$previous}
 [IO.File]::WriteAllLines("$out/live.tsv",[string[]]$live)
 if(Compare-Object @("105886`t4726`t0`t`tnpc_sr_argent_hippogryph","106205`t4726`t0`t`tnpc_sr_argent_hippogryph") $live){throw 'Summoned vehicle bindings changed; review rider admission'}
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TruthguardPickupCatalog.cpp -ProbeName TruthguardPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if($LASTEXITCODE){throw 'Rider DB2 probe failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $native=Get-Content "$out/native.tsv"
 foreach($expected in @("VEHICLE`t4726`t1073877039","SEAT`t4726`t0`t16558`t1073741831`t33554432`t768`t0`t0")){if($native -notcontains $expected){throw "Native vehicle/seat changed: $expected"}}
 if(@($native|Where-Object {$_ -like "SEAT`t4726`t*"}).Count -ne 1){throw 'Vehicle gained more seats; review identity admission'}
 $previous=$env:PYTHONPATH
 $checker=if($EndpointTest){'verify_truthguard_endpoint_release.py'}else{'verify_truthguard_rider_release.py'}
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/$checker" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed rider/endpoint guard failed'}}finally{$env:PYTHONPATH=$previous}
}
