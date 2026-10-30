param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$OutputDirectory='')
$ErrorActionPreference='Stop'
if($VerifyReleaseBinary -and ($Baseline -or $ScopeOnly)){throw 'Only repaired native checks certify Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth -and $last -lt $source.Length){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};if($depth){throw "Unclosed $start"};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'')
$pinned=(& git -C $repo show "956c72b97:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned first-path baseline unavailable'}
$original=Block $pinned 'class npc_travar_first : public CreatureScript'
$current=Block $source 'class npc_travar_first : public CreatureScript'
$hostCurrent=$current
if(!$Baseline){
 & "$PSScriptRoot/Test-SilverHandFirstRoom.ps1" -ScopeOnly
 $source=(& git -C $repo show "29f03cc0b:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned first-room scope unavailable'}
 $current=Block $source 'class npc_travar_first : public CreatureScript'
 $movement=Block $original 'void MovementInform('
 $movement=$movement.Replace((Block $movement 'if (id == 10)'), 'if (id == 10) { _firstPath.Observe(me); CompleteFirstPath(); }')
 if(($movement -replace '\s','') -cne ((Block $current 'void MovementInform(') -replace '\s','')){throw 'Unrelated first path movement changed'}
 $gossip=Block $original 'void sGossipSelect('
 $gossip=$gossip.Replace('me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);', 'if (!_firstPath.Request(me)) return; me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);')
 if(($gossip -replace '\s','') -cne ((Block $current 'void sGossipSelect(') -replace '\s','')){throw 'Unrelated first gossip changed'}
 $life=(Block $original 'void UpdateLifecycle(').Replace('CompleteFirstFight(diff);','_firstPath.Update(diff); CompleteFirstFight(diff);')
 if(($life -replace '\s','') -cne ((Block $current 'void UpdateLifecycle(') -replace '\s','')){throw 'Unrelated first lifecycle changed'}
 $update=(Block $original 'void UpdateAI(').Replace('_functions.Update(diff);','_functions.Update(diff); CompleteFirstPath();')
 if(($update -replace '\s','') -cne ((Block $current 'void UpdateAI(') -replace '\s','')){throw 'Unrelated first combat update changed'}
 $normalized=$current
 foreach($name in @('void MovementInform(','void sGossipSelect(','void UpdateLifecycle(','void UpdateAI(')){$normalized=$normalized.Replace((Block $normalized $name),(Block $original $name))}
 foreach($name in @('void CompleteFirstPath(','void DepartFirstPath(')){$normalized=$normalized.Replace((Block $normalized $name),'')}
 $normalized=$normalized.Replace('SilverHandFirstPath::State _firstPath;','')
 if(($normalized -replace '\s','') -cne ($original -replace '\s','')){throw 'Unrelated first Travard method/field changed'}
 $whole=$source.Replace($current,$original).Replace('#include "silver_hand_first_path.h"','')
 if(($whole -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Unrelated Silver Hand script changed'}
}
if($ScopeOnly){'PASS exact first-path-only scope; all other first-Travard and Silver Hand code retained';return}
$current=$hostCurrent
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/silver-hand-first-path"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$actor=if($Baseline){$original}else{$current}
$arrival=Block (Block $actor 'void MovementInform(') 'if (id == 10)'
$arrival=$arrival.Substring($arrival.IndexOf('{')+1);$arrival=$arrival.Substring(0,$arrival.LastIndexOf('}'))
[IO.File]::WriteAllText("$out/Arrival.inc",'void Arrive(){if(firstwp){'+$arrival+'}}')
$methods=if($Baseline){''}else{(@('void CompleteFirstPath(','void DepartFirstPath(','void UpdateLifecycle(','void Reset(')|ForEach-Object {Block $actor $_}) -join "`n"}
[IO.File]::WriteAllText("$out/Methods.inc",$methods)
$ordinary=Block $actor 'void UpdateAI('
$ordinary=$ordinary.Substring($ordinary.IndexOf('{')+1);$ordinary=$ordinary.Substring(0,$ordinary.IndexOf('if (!UpdateVictim())'))
[IO.File]::WriteAllText("$out/Ordinary.inc",'void Ordinary(uint32 diff){'+$ordinary+'}')
foreach($name in @('Scenario.h','ScenarioMgr.h','WaypointManager.h')){[IO.File]::WriteAllText("$out/$name",'// Controlled world declarations in the host; native policy is included unchanged.')}
$functions=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/FunctionClass.inc",(Block (Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw) 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';')
$names=@('FunctionProcessor::FunctionProcessor()','FunctionProcessor::~FunctionProcessor()','void FunctionProcessor::Update(','void FunctionProcessor::KillAllFunctions()','void FunctionProcessor::AddFunction(','void FunctionProcessor::AddFunctionsFromQueue()','uint64 FunctionProcessor::CalculateTime(','void FunctionProcessor::AddDelayedEvent(','uint32 FunctionProcessor::Size() const','uint32 FunctionProcessor::SizeQueue() const')
[IO.File]::WriteAllText("$out/FunctionMethods.inc",(($names|ForEach-Object {Block $functions $_}) -join "`n"))
$creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/Lifecycle.inc",(Block $creature 'if (i_AI || i_disabledAI)'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $definition /I`"$out`" /I`"$repo/src/server/scripts/Scenario/Artifacts/Paladin/SilverHand`" `"$PSScriptRoot/SilverHandFirstPathTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw "Native first-path regression failed: $LASTEXITCODE"}
if($VerifyReleaseBinary){
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SilverHandAllyCatalog.cpp -ProbeName SilverHandAllyCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native first-path catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 foreach($row in @("STEP`t1`t2444`t47896`t0`tClearing the Way","CRITERION`t1`t30017`t92`t50766`t0","FIRST_PATH_META`t30017`t0`t0`t47897`t0`t0")){if((Get-Content "$out/native.tsv") -notcontains $row){throw "Native first-path criterion changed: $row"}}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'binding',entry,ScriptName FROM creature_template WHERE entry=106429;
SELECT 'path',id,COUNT(*),MIN(point),MAX(point) FROM waypoint_data_script WHERE id IN(10993804,10993805) GROUP BY id ORDER BY id;
SELECT 'other-smart-producer',COUNT(*) FROM smart_scripts WHERE action_type=205 AND (action_param1=50766 OR action_param2=50766);
SELECT 'complete','first-path-readback';
'@
 [IO.File]::WriteAllText("$out/live-read.sql",$query)
 $previous=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$connection[3]
  $arguments=@("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names')
  $exitCode=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $arguments -InputFile "$out/live-read.sql" -OutputFile "$out/live.tsv" -ErrorFile "$out/live.stderr"
  if($exitCode -or (Get-Item "$out/live.stderr").Length){throw 'Full-EOF first-path readback failed'}
 }finally{$env:MYSQL_PWD=$previous}
 $expected=@("binding`t106429`tnpc_travar_first","path`t10993804`t11`t1`t11","path`t10993805`t19`t1`t19","other-smart-producer`t0","complete`tfirst-path-readback")
 if(Compare-Object $expected @(Get-Content "$out/live.tsv")){throw 'First-path binding/path/producer changed'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_first_path_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical first-path PE failed'}}finally{$env:PYTHONPATH=$previous}
}
