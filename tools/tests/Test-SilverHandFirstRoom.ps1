param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$OutputDirectory='')
$ErrorActionPreference='Stop'
if($VerifyReleaseBinary -and ($Baseline -or $ScopeOnly)){throw 'Only repaired native checks certify Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth -and $last -lt $source.Length){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};if($depth){throw "Unclosed $start"};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'')
$pinned=(& git -C $repo show "29f03cc0b:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned first-room baseline unavailable'}
$original=Block $pinned 'class npc_travar_first : public CreatureScript'
$current=Block $source 'class npc_travar_first : public CreatureScript'
if(!$Baseline){
 $movement=Block $original 'void MovementInform('
 $point2=Block $movement 'case 2:'
 $changed=$point2.Replace('DoCast(185414);','if (!_firstRoom.Begin(me, _firstFight.completed)) break; DoCast(185414);').Replace('Talk(20);','if (!_firstRoom.Issue(me)) return; Talk(20);')
 $movement=$movement.Replace($point2,$changed).Replace((Block $movement 'case 3:'),'case 3: { _firstRoom.Observe(me); CompleteFirstRoom(); }')
 if(($movement -replace '\s','') -cne ((Block $current 'void MovementInform(') -replace '\s','')){throw 'Unrelated first-room movement changed'}
 $life=(Block $original 'void UpdateLifecycle(').Replace('_firstPath.Update(diff);','_firstPath.Update(diff); _firstRoom.Update(diff);')
 if(($life -replace '\s','') -cne ((Block $current 'void UpdateLifecycle(') -replace '\s','')){throw 'Unrelated first-room lifecycle changed'}
 $update=(Block $original 'void UpdateAI(').Replace('CompleteFirstPath();','CompleteFirstPath(); CompleteFirstRoom();')
 if(($update -replace '\s','') -cne ((Block $current 'void UpdateAI(') -replace '\s','')){throw 'Unrelated first-room combat update changed'}
 $normalized=$current
 foreach($name in @('void MovementInform(','void UpdateLifecycle(','void UpdateAI(')){$normalized=$normalized.Replace((Block $normalized $name),(Block $original $name))}
 $normalized=$normalized.Replace((Block $normalized 'void CompleteFirstRoom('),'').Replace('SilverHandFirstRoom::State _firstRoom;','')
 if(($normalized -replace '\s','') -cne ($original -replace '\s','')){throw 'Unrelated first-Travard method/field changed'}
 $whole=$source.Replace($current,$original).Replace('#include "silver_hand_first_room.h"','')
 if(($whole -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Unrelated Silver Hand script changed'}
}
if($ScopeOnly){'PASS exact first-room-only scope; existing first-path/fight/other Silver Hand methods retained';return}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/silver-hand-first-room"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$actor=if($Baseline){$original}else{$current}
$movement=Block $actor 'void MovementInform('
$point2=Block $movement 'case 2:';$point3=Block $movement 'case 3:'
[IO.File]::WriteAllText("$out/Movement.inc",'void MovementInform(uint32 type,uint32 id)override{if(type!=POINT_MOTION_TYPE)return;switch(id){'+$point2+'break;'+$point3+'break;}}')
$names=@('void UpdateLifecycle(','void Reset(')
if(!$Baseline){$names+='void CompleteFirstRoom('}
[IO.File]::WriteAllText("$out/Methods.inc",(($names|ForEach-Object {Block $actor $_}) -join "`n"))
$ordinary=Block $actor 'void UpdateAI('
$ordinary=$ordinary.Substring($ordinary.IndexOf('{')+1);$ordinary=$ordinary.Substring(0,$ordinary.IndexOf('if (!UpdateVictim())'))
[IO.File]::WriteAllText("$out/Ordinary.inc",'void Ordinary(uint32 diff){'+$ordinary+'}')
foreach($name in @('Scenario.h','ScenarioMgr.h')){[IO.File]::WriteAllText("$out/$name",'// Controlled services precede the unchanged native policy in the host.')}
$functions=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/FunctionClass.inc",(Block (Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw) 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';')
$names=@('FunctionProcessor::FunctionProcessor()','FunctionProcessor::~FunctionProcessor()','void FunctionProcessor::Update(','void FunctionProcessor::KillAllFunctions()','void FunctionProcessor::AddFunction(','void FunctionProcessor::AddFunctionsFromQueue()','uint64 FunctionProcessor::CalculateTime(','void FunctionProcessor::AddDelayedEvent(','uint32 FunctionProcessor::Size() const','uint32 FunctionProcessor::SizeQueue() const')
[IO.File]::WriteAllText("$out/FunctionMethods.inc",(($names|ForEach-Object {Block $functions $_}) -join "`n"))
$creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/Lifecycle.inc",(Block $creature 'if (i_AI || i_disabledAI)'))
$point=Get-Content "$repo/src/server/game/Movement/MovementGenerators/PointMovementGenerator.cpp" -Raw
[IO.File]::WriteAllText("$out/PointFinalize.inc",'template<class T>'+(Block $point 'void PointMovementGenerator<T>::DoFinalize(')+"`ntemplate<>"+(Block $point 'void PointMovementGenerator<Creature>::MovementInform('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $definition /I`"$out`" /I`"$repo/src/server/scripts/Scenario/Artifacts/Paladin/SilverHand`" `"$PSScriptRoot/SilverHandFirstRoomTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw "Native first-room regression failed: $LASTEXITCODE"}
if($VerifyReleaseBinary){
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SilverHandAllyCatalog.cpp -ProbeName SilverHandAllyCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native first-room catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 foreach($row in @("STEP`t3`t2448`t47963`t0`tThe Broken Ritual","CRITERION`t3`t30073`t92`t50872`t0","FIRST_ROOM_META`t30073`t0`t0`t47964`t0`t0","CRITERION`t4`t30079`t92`t50897`t0","EFFECT`t185414`t0`t6`t4`t0`t0`t1`t0","EFFECT`t211502`t0`t6`t4`t0`t0`t1`t0")){if((Get-Content "$out/native.tsv") -notcontains $row){throw "Native first-room criterion/spell changed: $row"}}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'binding',entry,ScriptName FROM creature_template WHERE entry IN(106429,106639) ORDER BY entry;
SELECT 'next-spawn',guid,id,map,PhaseId FROM creature WHERE id=106639;
SELECT 'phase',pd.phaseId,co.ConditionTypeOrReference,co.ConditionValue1,co.ConditionValue2,co.NegativeCondition FROM phase_definitions pd JOIN conditions co ON co.SourceGroup=pd.zoneId AND co.SourceEntry=pd.entry AND co.SourceTypeOrReferenceId=23 WHERE pd.zoneId=85 AND pd.entry=7;
SELECT 'other-smart-producer',COUNT(*) FROM smart_scripts WHERE action_type=205 AND (action_param1=50872 OR action_param2=50872);
SELECT 'named-spells',COUNT(*) FROM spell_script_names WHERE spell_id IN(185414,211502);
SELECT 'complete','first-room-readback';
'@
 [IO.File]::WriteAllText("$out/live-read.sql",$query)
 $previous=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$connection[3]
  $arguments=@("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names')
  $exitCode=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $arguments -InputFile "$out/live-read.sql" -OutputFile "$out/live.tsv" -ErrorFile "$out/live.stderr"
  if($exitCode -or (Get-Item "$out/live.stderr").Length){throw 'Full-EOF first-room readback failed'}
 }finally{$env:MYSQL_PWD=$previous}
 $expected=@("binding`t106429`tnpc_travar_first","binding`t106639`tnpc_travar_second","next-spawn`t361991`t106639`t1539`t7000","phase`t7000`t44`t1092`t4`t0","other-smart-producer`t0","named-spells`t0","complete`tfirst-room-readback")
 if(Compare-Object $expected @(Get-Content "$out/live.tsv")){throw 'First-room binding/next phase/producer changed'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_first_room_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical first-room PE failed'}}finally{$env:PYTHONPATH=$previous}
}
