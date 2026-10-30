param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$OutputDirectory='')
$ErrorActionPreference='Stop'
if($VerifyReleaseBinary -and ($Baseline -or $ScopeOnly)){throw 'Only a repaired native run can verify Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$source,[string]$start){
 $first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1
 while($depth -and $last -lt $source.Length){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last}
 if($depth){throw "Unclosed $start"};$source.Substring($first,$last-$first)
}
$path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'')
$pinned=(& git -C $repo show "ee5d4b981:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned first-fight baseline unavailable'}
$original=Block $pinned 'class npc_travar_first : public CreatureScript'
$current=Block $source 'class npc_travar_first : public CreatureScript'
$hostCurrent=$current
if(!$Baseline){
 & "$PSScriptRoot/Test-SilverHandFirstPath.ps1" -ScopeOnly
 $pathPinned=(& git -C $repo show "956c72b97:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned first-path scope unavailable'}
 $current=Block $pathPinned 'class npc_travar_first : public CreatureScript'
 $movement=Block $original 'void MovementInform('
 $actual=Block $current 'void MovementInform('
 $expected=@'
if (id == 18) { if (!_firstFight.Begin(me)) return; Talk(17); SpawnFirstFight(); }
'@
 $movement=$movement.Replace((Block $movement 'if (id == 18)'),$expected)
 if(($movement -replace '\s','') -cne ($actual -replace '\s','')){throw 'Unrelated first-Travard movement changed'}
 $life=Block $original 'void UpdateLifecycle('
 $life=$life.Replace('            _entryFunctions.Update(diff);',"            CompleteFirstFight(diff);`n            _entryFunctions.Update(diff);")
 if(($life -replace '\s','') -cne ((Block $current 'void UpdateLifecycle(') -replace '\s','')){throw 'Unrelated first lifecycle changed'}
 $normalized=$current
 foreach($name in @('void EnterEvadeMode(','void MovementInform(','void UpdateLifecycle(')){$normalized=$normalized.Replace((Block $normalized $name),(Block $original $name))}
 foreach($name in @('void JustSummoned(','void SummonedCreatureDies(','void SetGUID(','void SpawnFirstFight(','void CompleteFirstFight(')){$normalized=$normalized.Replace((Block $normalized $name),'')}
 $normalized=$normalized.Replace('SilverHandFirstFight::State _firstFight;','')
 if(($normalized -replace '\s','') -cne ($original -replace '\s','')){throw 'Unrelated entrance/ally/quest/scene code changed'}
 $instancePath='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/instance_silver_hand.cpp'
 $instance=Get-Content "$repo/$instancePath" -Raw
 $instancePinned=(& git -C $repo show "ee5d4b981:$instancePath") -join "`n";if($LASTEXITCODE){throw 'Pinned instance scope unavailable'}
 $instanceNormalized=$instance.Replace((Block $instance 'void CreatureDies('),'').Replace('#include "silver_hand_first_fight.h"','')
 if(($instanceNormalized -replace '\s','') -cne ($instancePinned -replace '\s','')){throw 'Unrelated instance ritual/phase/arrival changed'}
}
if($ScopeOnly){'PASS exact first-fight-only class scope; entrance/admission/heal/reset/other route actions retained';return}
$current=$hostCurrent
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/silver-hand-first-fight"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$actor=if($Baseline){$original}else{$current}
$start=Block (Block $actor 'void MovementInform(') 'if (id == 18)'
$start=$start.Substring($start.IndexOf('{')+1);$start=$start.Substring(0,$start.LastIndexOf('}'))
if($Baseline){
 [IO.File]::WriteAllText("$out/Start.inc",$start)
 [IO.File]::WriteAllText("$out/Evade.inc",((Block $actor 'void EnterEvadeMode(')+"`n"+(Block $actor 'void UpdateLifecycle(')+"`n"+(Block $actor 'void Reset(')))
}else{
 [IO.File]::WriteAllText("$out/InstanceDeath.inc",(Block $instance 'void CreatureDies('))
 $methods=foreach($name in @('void EnterEvadeMode(','void JustSummoned(','void SummonedCreatureDies(','void SetGUID(','void SpawnFirstFight(','void CompleteFirstFight(','void UpdateLifecycle(','void Reset(')){Block $actor $name}
 [IO.File]::WriteAllText("$out/Methods.inc",($methods -join "`n")+"`nvoid Start(){"+$start+"`n}")
 foreach($name in @('Scenario.h','ScenarioMgr.h','TemporarySummon.h')){[IO.File]::WriteAllText("$out/$name",'// Controlled world declarations are in the host before the production header.')}
}
$functions=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/FunctionClass.inc",(Block (Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw) 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';')
$methods=@('FunctionProcessor::FunctionProcessor()','FunctionProcessor::~FunctionProcessor()','void FunctionProcessor::Update(','void FunctionProcessor::KillAllFunctions()','void FunctionProcessor::AddFunction(','void FunctionProcessor::AddFunctionsFromQueue()','uint64 FunctionProcessor::CalculateTime(','void FunctionProcessor::AddDelayedEvent(','uint32 FunctionProcessor::Size() const','uint32 FunctionProcessor::SizeQueue() const')
[IO.File]::WriteAllText("$out/FunctionMethods.inc",(($methods|ForEach-Object{Block $functions $_}) -join "`n"))
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$first=$unit.IndexOf('        // Call creature just died function',$unit.IndexOf('void Unit::Kill('));$last=$unit.IndexOf('        // Dungeon specific stuff',$first)
if($first -lt 0 -or $last -le $first){throw 'Native death dispatch boundary missing'}
[IO.File]::WriteAllText("$out/DeathBridge.inc",$unit.Substring($first,$last-$first))
$summon=Get-Content "$repo/src/server/game/Entities/Creature/TemporarySummon.cpp" -Raw
[IO.File]::WriteAllText("$out/InitSummon.inc",(Block $summon 'void TempSummon::InitSummon()'))
$creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/DisabledAI.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'UnitAI* GetDisabledAI() const'))
[IO.File]::WriteAllText("$out/Lifecycle.inc",(Block $creature 'if (i_AI || i_disabledAI)'))
$guid=Get-Content "$repo/src/server/game/Entities/Object/ObjectGuid.cpp" -Raw
[IO.File]::WriteAllText("$out/Guid.inc",((@('ObjectGuid::ObjectGuid()','void ObjectGuid::Clear()','bool ObjectGuid::IsEmpty() const')|ForEach-Object{Block $guid $_}) -join "`n"))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $definition /I`"$out`" /I`"$repo/src/server/scripts/Scenario/Artifacts/Paladin/SilverHand`" `"$PSScriptRoot/SilverHandFirstFightTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw "Native first-fight regression failed: $LASTEXITCODE"}
if($VerifyReleaseBinary){
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SilverHandAllyCatalog.cpp -ProbeName SilverHandAllyCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native first-fight catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 foreach($row in @("CRITERION`t2`t30046`t92`t50850`t0","CRITERION`t3`t30073`t92`t50872`t0","EFFECT`t208257`t0`t6`t4`t0`t0`t1`t0")){if((Get-Content "$out/native.tsv") -notcontains $row){throw "Native first-fight criterion/spell changed: $row"}}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'binding',entry,AIName,ScriptName FROM creature_template WHERE entry IN(106429,106624) ORDER BY entry;
SELECT 'spawn',guid,id,map,PhaseId FROM creature WHERE id IN(106429,106624) AND map=1539 ORDER BY id;
SELECT 'instance',map,script FROM instance_template WHERE map=1539;
SELECT 'path',id,COUNT(*) FROM waypoint_data_script WHERE id=10993805 GROUP BY id;
SELECT 'scenario',ScenarioID,Class FROM scenario_data WHERE MapID=1539 ORDER BY ScenarioID;
SELECT 'complete','first-fight-readback';
'@
 [IO.File]::WriteAllText("$out/live-read.sql",$query)
 $previous=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$connection[3]
  $arguments=@("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names')
  $exitCode=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $arguments -InputFile "$out/live-read.sql" -OutputFile "$out/live.tsv" -ErrorFile "$out/live.stderr"
  if($exitCode -or (Get-Item "$out/live.stderr").Length){throw 'Full-EOF first-fight live readback failed'}
 }finally{$env:MYSQL_PWD=$previous}
 $expected=@("binding`t106429`t`tnpc_travar_first","binding`t106624`tSmartAI`t","spawn`t361895`t106429`t1539`t5738 7005","instance`t1539`tinstance_silver_hand","path`t10993805`t19","scenario`t991`t5","scenario`t1037`t1","scenario`t1092`t2","complete`tfirst-fight-readback")
 if(Compare-Object $expected @(Get-Content "$out/live.tsv")){throw 'First-fight actual binding/instance/path/shared-scenario readback differs'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_first_fight_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical first-fight PE failed'}}finally{$env:PYTHONPATH=$previous}
}
