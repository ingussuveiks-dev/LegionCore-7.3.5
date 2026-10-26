param([switch]$Baseline,[switch]$AdmissionBaseline,[switch]$DeathBaseline,[switch]$ProgressBaseline,[switch]$ContextBaseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if(@($Baseline,$AdmissionBaseline,$DeathBaseline,$ProgressBaseline,$ContextBaseline|Where-Object {$_}).Count -gt 1 -or (($Baseline -or $AdmissionBaseline -or $DeathBaseline -or $ProgressBaseline -or $ContextBaseline) -and $VerifyReleaseBinary)){throw 'Pinned baselines are exclusive and cannot certify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/silver-hand-wave-lifetime"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp'
$source=Get-Content "$repo/$path" -Raw
$pinned=(& git -C $repo show "e03ec0c8:$path") -join "`n"
if($LASTEXITCODE){throw 'Pinned last Travard baseline unavailable'}
$admissionPinned=(& git -C $repo show "67b2f259:$path") -join "`n"
if($LASTEXITCODE){throw 'Pinned final entry baseline unavailable'}
$deathPinned=(& git -C $repo show "1f73b44c:$path") -join "`n"
if($LASTEXITCODE){throw 'Pinned wave death baseline unavailable'}
$progressPinned=(& git -C $repo show "df5bf3f5:$path") -join "`n"
if($LASTEXITCODE){throw 'Pinned ritual progress baseline unavailable'}
$contextPinned=(& git -C $repo show "f3e7c9e2:$path") -join "`n"
if($LASTEXITCODE){throw 'Pinned pending first-wave context baseline unavailable'}
$actor=Block $(if($Baseline){$pinned}elseif($AdmissionBaseline){$admissionPinned}elseif($DeathBaseline){$deathPinned}elseif($ProgressBaseline){$progressPinned}elseif($ContextBaseline){$contextPinned}else{$source}) 'struct npc_travar_lastAI : ScriptedAI'
if(!$Baseline -and !$AdmissionBaseline -and !$DeathBaseline -and !$ProgressBaseline -and !$ContextBaseline){
 $oldContext=Block $contextPinned 'struct npc_travar_lastAI : ScriptedAI'
 $waveGuard=Block $actor 'if (!SilverHandFinalEntry::CanStartWave(me))'
 $expectedGuard='if(!SilverHandFinalEntry::CanStartWave(me)){_introDone=false;startCriteria=false;TC_LOG_DEBUG("scripts","Silver Hand ritual: pending first wave context changed; rearming entry without spawning");return;}'
 if(($waveGuard -replace '\s','') -cne ($expectedGuard -replace '\s','') -or (($actor.Replace($waveGuard,'') -replace '\s','') -cne ($oldContext -replace '\s',''))){throw 'Pending first-wave change exceeds live context guard and entry rearm'}
 $policy=Block $source 'namespace SilverHandFinalEntry'
 if(($policy.Replace((Block $policy 'bool CanStartWave('),'') -replace '\s','') -cne ((Block $contextPinned 'namespace SilverHandFinalEntry') -replace '\s','')){throw 'Existing entry/progress policy changed'}
 $comparisonContext=$actor.Replace($waveGuard,'')
 $oldProgress=Block (Block $progressPinned 'struct npc_travar_lastAI : ScriptedAI') 'void UpdateAI('
 $currentProgress=Block $actor 'void UpdateAI('
 if(($currentProgress -replace '\s','') -cne (($oldProgress -replace '\s','').Replace('me->FindNearestPlayer(100.0f)','SilverHandFinalEntry::SelectPlayer(me)'))){throw 'Ritual progress changed beyond scoped recipient selection'}
 $comparisonProgress=$comparisonContext.Replace($currentProgress,$oldProgress)
 if(((Block (Block $source 'namespace SilverHandFinalEntry') 'bool CanStart(') -replace '\s','') -cne ((Block (Block $progressPinned 'namespace SilverHandFinalEntry') 'bool CanStart(') -replace '\s','')){throw 'Original final-entry eligibility changed'}
 $beforeDeaths=Block $deathPinned 'struct npc_travar_lastAI : ScriptedAI'
 $deathNormalized=$comparisonProgress.Replace('GuidSet _waveSummons;','').Replace((Block $actor 'void JustSummoned('),'').Replace((Block $actor 'void SummonedCreatureDies('),(Block $beforeDeaths 'void SummonedCreatureDies('))
 $deathNormalized=$deathNormalized.Replace('JustSummoned(targ);','').Replace('JustSummoned(add);','')
 $newAddBranch=Block $deathNormalized 'if (Creature* add = me->SummonCreature(106578'
 $oldAddBranch=$newAddBranch.Replace('{','').Replace('}','')
 $deathNormalized=$deathNormalized.Replace($newAddBranch,$oldAddBranch)
 if(($deathNormalized -replace '\s','') -cne ($beforeDeaths -replace '\s','')){throw 'Last AI changed beyond scoped summon registration/death admission'}
 $comparisonActor=$deathNormalized
 $original=Block $pinned 'struct npc_travar_lastAI : ScriptedAI'
 $originalLos=Block $original 'void MoveInLineOfSight('
 $currentLos=Block $comparisonActor 'void MoveInLineOfSight('
 $expectedLos=($originalLos -replace '\s','').Replace('if(who->GetTypeId()!=TYPEID_PLAYER)return;if(!_introDone)','if(_introDone)return;Player*player=who?who->ToPlayer():nullptr;if(SilverHandFinalEntry::CanStart(player,me))').Replace('if(Player*pl=me->FindNearestPlayer(100.0f))pl->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,50915);','').Replace('me->AddDelayedEvent(5000','_waveFunctions.AddDelayedEvent(5000')
 if(($currentLos -replace '\s','') -cne $expectedLos){throw 'Final entry changed outside scoped admission and removal of earlier-stage50915'}
 $actorForComparison=$comparisonActor.Replace($currentLos,$originalLos.Replace('me->AddDelayedEvent(5000','_waveFunctions.AddDelayedEvent(5000'))
 $normalized=$actorForComparison.Replace('FunctionProcessor _waveFunctions;','').Replace((Block $actor 'void UpdateLifecycle('),'').Replace('_waveFunctions.AddDelayedEvent(5000','me->AddDelayedEvent(5000')
 if(($normalized -replace '\s','') -cne ($original -replace '\s','')){throw 'Unrelated last Travard behavior changed'}
 if($actor.Contains('me->AddDelayedEvent(')){throw 'Unit-owned last Travard AI timer remains'}
}
[IO.File]::WriteAllText("$out/WaveActor.inc",$actor+';')
[IO.File]::WriteAllText("$out/FinalEntryPolicy.inc",$(if($Baseline -or $AdmissionBaseline){''}elseif($ProgressBaseline){Block $progressPinned 'namespace SilverHandFinalEntry'}elseif($ContextBaseline){Block $contextPinned 'namespace SilverHandFinalEntry'}else{Block $source 'namespace SilverHandFinalEntry'}))
$functions=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/FunctionClass.inc",(Block (Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw) 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';')
$methods=@('FunctionProcessor::FunctionProcessor()','FunctionProcessor::~FunctionProcessor()','void FunctionProcessor::Update(','void FunctionProcessor::KillAllFunctions()','void FunctionProcessor::AddFunction(','void FunctionProcessor::AddFunctionsFromQueue()','uint64 FunctionProcessor::CalculateTime(','void FunctionProcessor::AddDelayedEvent(','uint32 FunctionProcessor::Size() const','uint32 FunctionProcessor::SizeQueue() const')
[IO.File]::WriteAllText("$out/FunctionMethods.inc",(($methods|ForEach-Object{Block $functions $_}) -join "`n"))
$creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/LifecycleBridge.inc",(Block $creature 'if (i_AI || i_disabledAI)'))
[IO.File]::WriteAllText("$out/DestroyAI.inc",(Block $creature 'bool Creature::AIM_Destroy()'))
$initialization=Block (Get-Content "$repo/src/server/game/Entities/Creature/TemporarySummon.cpp" -Raw) 'void TempSummon::InitSummon()'
if($initialization.IndexOf('JustSummoned(this)') -lt 0 -or $initialization.IndexOf('JustSummoned(this)') -gt $initialization.IndexOf('IsSummonedBy(owner)')){throw 'Native owner registration no longer precedes summon AI callback'}
[IO.File]::WriteAllText("$out/InitSummon.inc",$initialization)
$creation=Block (Get-Content "$repo/src/server/game/Maps/Map.cpp" -Raw) 'TempSummon* Map::SummonCreature('
if($creation.IndexOf('AddToMap(summon->ToCreature())') -lt 0 -or $creation.IndexOf('AddToMap(summon->ToCreature())') -gt $creation.IndexOf('summon->InitSummon()') -or $creation.IndexOf('summon->InitSummon()') -gt $creation.IndexOf('return summon;')){throw 'Native live-map/registration/return order changed'}
$kill=Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw) 'void Unit::Kill('
$bridgeFirst=$kill.IndexOf('        if (TempSummon* summon = creature->ToTempSummon())')
$bridgeLast=$kill.IndexOf('        // Call instance script.',$bridgeFirst)
if($bridgeFirst -lt 0 -or $bridgeLast -le $bridgeFirst){throw 'Native unbraced owner death bridge missing'}
$deathBridge=$kill.Substring($bridgeFirst,$bridgeLast-$bridgeFirst)
if($kill.IndexOf('victim->setDeathState(JUST_DIED);') -lt 0 -or $kill.IndexOf('victim->setDeathState(JUST_DIED);') -gt $kill.IndexOf($deathBridge)){throw 'Native owner death notification precedes death state'}
[IO.File]::WriteAllText("$out/DeathBridge.inc",$deathBridge.Replace('creature, this','creature, killer'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}elseif($AdmissionBaseline){'/DADMISSION_BASELINE'}elseif($DeathBaseline){'/DDEATH_BASELINE'}elseif($ProgressBaseline){'/DPROGRESS_BASELINE'}elseif($ContextBaseline){'/DCONTEXT_BASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/SilverHandWaveLifetimeTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native last Travard wave lifetime regression failed'}
if($Baseline -or $AdmissionBaseline -or $DeathBaseline -or $ProgressBaseline -or $ContextBaseline){return}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SilverHandPickupCatalog.cpp -ProbeName SilverHandPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native late scenario metadata failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
foreach($expected in @("CRITERION`t8`t30089`t92`t50915`t0","CRITERION`t9`t30110`t92`t51838`t1","CRITERION`t10`t30111`t92`t50966`t0")){if((Get-Content "$out/native.tsv") -notcontains $expected){throw "Native late criterion changed: $expected"}}
foreach($expected in @("TREE`t9`t48055`t0`t180`t4`t32`t0","TREE`t9`t53744`t48055`t180`t9`t32`t0","TREE`t9`t53745`t53744`t1`t0`t0`t30110","CRITERION_META`t30110`t0`t0`t0`t0`t0`t0`t0")){if((Get-Content "$out/native.tsv") -notcontains $expected){throw "Native ritual weighted tree changed: $expected"}}
$achievement=Get-Content "$repo/src/server/game/Achievements/AchievementMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/TreeOperators.inc",(Block (Get-Content "$repo/src/server/game/DataStores/DBCEnums.h" -Raw) 'enum CriteriaTreeOperator')+';')
[IO.File]::WriteAllText("$out/TreeCompletion.inc",'template<typename Func>'+"`n"+(Block $achievement 'void AchievementGlobalMgr::WalkCriteriaTree(')+"`ntemplate<class T>`n"+(Block $achievement 'bool AchievementMgr<T>::IsCompletedCriteriaTree('))
[IO.File]::WriteAllText("$out/tree.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/SilverHandRitualTreeTest.cpp`" /Fe`"$out/tree.exe`" /Fo`"$out/tree.obj`"`nif errorlevel 1 exit /b 1`n`"$out/tree.exe`" `"$out/native.tsv`"`n")
& "$out/tree.cmd";if($LASTEXITCODE){throw 'Native ritual weighted completion regression failed'}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$live=@("SET SESSION TRANSACTION READ ONLY;SELECT 'binding',entry,ScriptName FROM creature_template WHERE ScriptName='npc_travar_last';SELECT 'spawn',guid,id,map,PhaseId FROM creature WHERE id=106777;SELECT 'phase',pd.phaseId,co.ConditionTypeOrReference,co.ConditionValue1,co.ConditionValue2,co.ElseGroup FROM phase_definitions pd JOIN conditions co ON co.SourceGroup=pd.zoneId AND co.SourceEntry=pd.entry AND co.SourceTypeOrReferenceId=23 WHERE pd.zoneId=85 AND pd.entry=10;SELECT 'aura',entry,auras FROM creature_template_addon WHERE entry=106777;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Last Travard binding query failed'}}finally{$env:MYSQL_PWD=$previous}
 $expected=@("binding`t106777`tnpc_travar_last","spawn`t362048`t106777`t1539`t7003","phase`t7003`t44`t1092`t9`t0","phase`t7003`t44`t1092`t10`t1","aura`t106777`t215258")
 if(Compare-Object $expected $live){throw 'Native last Travard actor/phase/aura changed'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_ritual_progress_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed ritual progress/wave death/final entry/lifetime repair missing'}}finally{$env:PYTHONPATH=$previous}
}
