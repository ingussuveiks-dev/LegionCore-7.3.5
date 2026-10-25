param([switch]$Baseline,[switch]$VerifyReleaseBinary,[switch]$PickupAdmission,[switch]$PickupBaseline)
$ErrorActionPreference='Stop'
if(($Baseline -or $PickupBaseline) -and $VerifyReleaseBinary){throw 'Select baseline without current Release verification'}
if($Baseline -and $PickupBaseline){throw 'Select one baseline'}
if($PickupBaseline){$PickupAdmission=$true}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/earthwarder-return"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/EarthWarder.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline -or $PickupBaseline){$revision=if($PickupBaseline){'3a38903e'}else{'b5c32cab'};$source=(& git -C $repo show "${revision}:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Earthwarder source unavailable'}}
[IO.File]::WriteAllText("$out/Policy.inc",$(if($Baseline){''}else{Block $source 'namespace EarthwarderReturn'}))
[IO.File]::WriteAllText("$out/PickupPolicy.inc",$(if($source.Contains('namespace EarthwarderAcquisition')){Block $source 'namespace EarthwarderAcquisition'}else{''}))
[IO.File]::WriteAllText("$out/Callback.inc",(Block $source 'class go_art_earthwarder')+';')
$shared=Get-Content "$repo/src/server/game/DataStores/DBCEnums.h" -Raw
[IO.File]::WriteAllText("$out/CriteriaTypes.inc",(Block $shared 'enum CriteriaTypes')+';')
$scenario=Get-Content "$repo/src/server/game/Scenarios/Scenario.cpp" -Raw
[IO.File]::WriteAllText("$out/Completed.inc",(Block $scenario 'bool Scenario::IsCompleted('))
[IO.File]::WriteAllText("$out/Count.inc",(Block $scenario 'uint8 Scenario::GetStepCount('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline -or $PickupBaseline){'/DBASELINE'}else{''}
if($PickupAdmission){$definition+=' /DPICKUP_ADMISSION'}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/EarthwarderReturnTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Actual Earthwarder return handler failed'}
if($VerifyReleaseBinary){
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarswordsCreditCatalog.cpp -ProbeName WarswordsCreditCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Earthwarder native completion read failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $steps=@(Get-Content "$out/native.tsv"|Where-Object {$_ -like "EARTH_STEP`t*"})
 if($steps.Count -ne 6 -or @($steps|Where-Object {$_ -notmatch '^EARTH_STEP\t[0-5]\t\d+\t0\t0\t'}).Count){throw 'Native909 completion count/reward changed'}
 $identities=@(1856,1828,1829,1830,1869,1833)
 for($order=0;$order -lt $identities.Count;++$order){if(@($steps|Where-Object {$_ -like "EARTH_STEP`t$order`t$($identities[$order])`t0`t0`t*"}).Count -ne 1){throw "Native909 ordered step $order changed"}}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$rows=@(@'
SELECT 'objective',ID,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=39191 ORDER BY ID;
SELECT 'weapon',guid,id,map FROM gameobject WHERE id=248831;
SELECT 'receiver',guid,id,map,position_x,position_y,position_z,PhaseId FROM creature WHERE guid=369004;
SELECT 'credit',action_type,action_param1,target_type,target_param1,event_param3,event_param4 FROM smart_scripts WHERE entryorguid=93823 AND source_type=0 AND id=10;
SELECT 'destination',target_map,target_position_x,target_position_y,target_position_z FROM spell_target_position WHERE id=216016;
'@|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Earthwarder native receiver read failed'}}finally{$env:MYSQL_PWD=$previous}
 [IO.File]::WriteAllLines("$out/receiver.tsv",[string[]]$rows)
 foreach($expected in @("objective`t279784`t0`t96508`t1`t0","objective`t279785`t1`t94851`t1`t2","objective`t282129`t2`t103739`t1`t2","weapon`t134175`t248831`t1495","receiver`t369004`t93823`t1479`t1059.61`t7224.6`t100.378`t5997","credit`t33`t103739`t18`t50`t2000`t2000","destination`t1479`t1028.64`t7225.23`t100.18")){if($rows -notcontains $expected){throw "Native Earthwarder receiver differs: $expected; rows=$($rows.Count)"}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_earthwarder_return_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Earthwarder return guard missing'}}finally{$env:PYTHONPATH=$previous}
}
