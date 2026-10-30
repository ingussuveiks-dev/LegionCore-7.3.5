param([switch]$Baseline,[switch]$ArrivalBaseline,[switch]$LifetimeBaseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if(@(@($Baseline,$ArrivalBaseline,$LifetimeBaseline)|Where-Object {$_}).Count -gt 1 -or (($Baseline -or $ArrivalBaseline -or $LifetimeBaseline) -and $VerifyReleaseBinary)){throw 'Baseline cannot certify fixed Release or combine baselines'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/silver-hand-entrance"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "4685321c:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned entrance baseline unavailable'}}
if($ArrivalBaseline){$source=(& git -C $repo show "c5eb41f3:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned arrival baseline unavailable'}}
if($LifetimeBaseline){$source=(& git -C $repo show "1237db9f:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned entrance lifetime baseline unavailable'}}
[IO.File]::WriteAllText("$out/EntrancePolicy.inc",$(if($Baseline){''}else{Block $source 'namespace SilverHandEntrance'}))
$actor=Block $source 'struct npc_travar_firstAI : ScriptedAI'
[IO.File]::WriteAllText("$out/EntranceLos.inc",(Block $actor 'void MoveInLineOfSight(').Replace(' override',''))
[IO.File]::WriteAllText("$out/EntranceReset.inc",$(if($Baseline -or $ArrivalBaseline -or $LifetimeBaseline){''}else{Block $actor 'void Reset('}))
[IO.File]::WriteAllText("$out/EntranceLifecycle.inc",$(if($Baseline -or $ArrivalBaseline -or $LifetimeBaseline){''}else{Block $actor 'void UpdateLifecycle('}))
$functions=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/FunctionClass.inc",(Block (Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw) 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';')
$methods=@('FunctionProcessor::FunctionProcessor()','FunctionProcessor::~FunctionProcessor()','void FunctionProcessor::Update(','void FunctionProcessor::KillAllFunctions()','void FunctionProcessor::AddFunction(','void FunctionProcessor::AddFunctionsFromQueue()','uint64 FunctionProcessor::CalculateTime(','void FunctionProcessor::AddDelayedEvent(','uint32 FunctionProcessor::Size() const','uint32 FunctionProcessor::SizeQueue() const')
[IO.File]::WriteAllText("$out/FunctionMethods.inc",(($methods|ForEach-Object{Block $functions $_}) -join "`n"))
$creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/LifecycleBridge.inc",(Block $creature 'if (i_AI || i_disabledAI)'))
[IO.File]::WriteAllText("$out/DestroyAI.inc",(Block $creature 'bool Creature::AIM_Destroy()'))
$movement=Block $actor 'void MovementInform('
$fightPrefix=if($Baseline -or $ArrivalBaseline -or $LifetimeBaseline){'me->AddDelayedEvent(2000,'}else{'_entryFunctions.AddDelayedEvent(2000,'}
$fightSource=if(!$Baseline -and !$ArrivalBaseline -and !$LifetimeBaseline){Block $actor 'void SpawnFirstFight('}else{$movement}
$fightGeneration=if(!$Baseline -and !$ArrivalBaseline -and !$LifetimeBaseline){'uint64 generation = _firstFight.Generation();'}else{''}
[IO.File]::WriteAllText("$out/EntranceFight.inc",'void ArmFirstFight(){'+$fightGeneration+(Block $fightSource $fightPrefix)+');}')
if(!$Baseline -and !$ArrivalBaseline -and !$LifetimeBaseline -and $actor.Contains('me->AddDelayedEvent(')){throw 'First Travard still queues a raw-AI timer on Unit'}
$first=$movement.IndexOf('case 1:');$last=$movement.IndexOf('case 2:',$first)
if($first -lt 0 -or $last -le $first){throw 'POINT1 arrival dispatch missing'}
$arrival=$movement.Substring($first,$last-$first)
if($ArrivalBaseline){$arrival='void CompleteEntranceArrival(){'+$arrival.Substring('case 1:'.Length).Substring(0,$arrival.Substring('case 1:'.Length).LastIndexOf('break;'))+'}'}
elseif(!$Baseline){if($arrival -notmatch '^case 1:\s+CompleteEntranceArrival\(\);\s+break;\s*$'){throw 'Actual POINT1 does not dispatch arrival helper'};$arrival=Block $actor 'void CompleteEntranceArrival('}
else{$arrival=''}
[IO.File]::WriteAllText("$out/EntranceArrival.inc",$arrival)
$guid=Get-Content "$repo/src/server/game/Entities/Object/ObjectGuid.cpp" -Raw
[IO.File]::WriteAllText("$out/GuidLifecycle.inc",((@('ObjectGuid::ObjectGuid()','void ObjectGuid::Clear()','bool ObjectGuid::IsEmpty() const')|ForEach-Object{Block $guid $_}) -join "`n"))
$header=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.h" -Raw
[IO.File]::WriteAllText("$out/WorldPlayer.inc",(Block $header 'static Player* GetObjectInWorld(ObjectGuid'))
[IO.File]::WriteAllText("$out/MapPlayer.inc",(Block $header 'template<class T> static T* GetObjectInMap('))
[IO.File]::WriteAllText("$out/AccessorPlayer.inc",(Block (Get-Content "$repo/src/server/game/Globals/ObjectAccessor.cpp" -Raw) 'Player* ObjectAccessor::GetPlayer('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}elseif($ArrivalBaseline){'/DARRIVAL_BASELINE'}elseif($LifetimeBaseline){'/DLIFETIME_BASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/SilverHandEntranceTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native entrance regression failed'}
if($Baseline -or $ArrivalBaseline -or $LifetimeBaseline){return}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SilverHandPickupCatalog.cpp -ProbeName SilverHandPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native entrance catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$rows=Get-Content "$out/native.tsv"
foreach($expected in @("CRITERION`t0`t29843`t92`t50765`t0","CRITERION`t1`t30017`t92`t50766`t0","DALARAN_CREDIT_SPELL`t210360`t0`t90`tReturn to Dalaran","EFFECT`t210360`t0`t90`t0`t105892`t0`t1`t0","EFFECT`t210360`t1`t252`t0`t1317`t0`t1`t17")){if($rows -notcontains $expected){throw "Native entrance/return research changed: $expected"}}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$live=@("SET SESSION TRANSACTION READ ONLY;SELECT 'binding',entry,ScriptName FROM creature_template WHERE ScriptName='npc_travar_first';SELECT 'spawn',guid,id,map,PhaseId FROM creature WHERE id=106429;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Entrance binding query failed'}}finally{$env:MYSQL_PWD=$previous}
 if($live.Count -ne 2 -or $live -notcontains "binding`t106429`tnpc_travar_first" -or $live -notcontains "spawn`t361895`t106429`t1539`t5738 7005"){throw 'Shared entrance binding/spawn changed'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_arrival_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed entrance/arrival guard missing'}}finally{$env:PYTHONPATH=$previous}
}
