param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot certify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/silver-hand-wave-lifetime"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp'
$source=Get-Content "$repo/$path" -Raw
$pinned=(& git -C $repo show "e03ec0c8:$path") -join "`n"
if($LASTEXITCODE){throw 'Pinned last Travard baseline unavailable'}
$actor=Block $(if($Baseline){$pinned}else{$source}) 'struct npc_travar_lastAI : ScriptedAI'
if(!$Baseline){
 $original=Block $pinned 'struct npc_travar_lastAI : ScriptedAI'
 $normalized=$actor.Replace('FunctionProcessor _waveFunctions;','').Replace((Block $actor 'void UpdateLifecycle('),'').Replace('_waveFunctions.AddDelayedEvent(5000','me->AddDelayedEvent(5000')
 if(($normalized -replace '\s','') -cne ($original -replace '\s','')){throw 'Unrelated last Travard behavior changed'}
 if($actor.Contains('me->AddDelayedEvent(')){throw 'Unit-owned last Travard AI timer remains'}
}
[IO.File]::WriteAllText("$out/WaveActor.inc",$actor+';')
$functions=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/FunctionClass.inc",(Block (Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw) 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';')
$methods=@('FunctionProcessor::FunctionProcessor()','FunctionProcessor::~FunctionProcessor()','void FunctionProcessor::Update(','void FunctionProcessor::KillAllFunctions()','void FunctionProcessor::AddFunction(','void FunctionProcessor::AddFunctionsFromQueue()','uint64 FunctionProcessor::CalculateTime(','void FunctionProcessor::AddDelayedEvent(','uint32 FunctionProcessor::Size() const','uint32 FunctionProcessor::SizeQueue() const')
[IO.File]::WriteAllText("$out/FunctionMethods.inc",(($methods|ForEach-Object{Block $functions $_}) -join "`n"))
$creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/LifecycleBridge.inc",(Block $creature 'if (i_AI || i_disabledAI)'))
[IO.File]::WriteAllText("$out/DestroyAI.inc",(Block $creature 'bool Creature::AIM_Destroy()'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/SilverHandWaveLifetimeTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native last Travard wave lifetime regression failed'}
if($Baseline){return}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SilverHandPickupCatalog.cpp -ProbeName SilverHandPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native late scenario metadata failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
foreach($expected in @("CRITERION`t8`t30089`t92`t50915`t0","CRITERION`t9`t30110`t92`t51838`t1","CRITERION`t10`t30111`t92`t50966`t0")){if((Get-Content "$out/native.tsv") -notcontains $expected){throw "Native late criterion changed: $expected"}}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$live=@("SET SESSION TRANSACTION READ ONLY;SELECT 'binding',entry,ScriptName FROM creature_template WHERE ScriptName='npc_travar_last';SELECT 'spawn',guid,id,map,PhaseId FROM creature WHERE id=106777;SELECT 'phase',pd.phaseId,co.ConditionTypeOrReference,co.ConditionValue1,co.ConditionValue2,co.ElseGroup FROM phase_definitions pd JOIN conditions co ON co.SourceGroup=pd.zoneId AND co.SourceEntry=pd.entry AND co.SourceTypeOrReferenceId=23 WHERE pd.zoneId=85 AND pd.entry=10;SELECT 'aura',entry,auras FROM creature_template_addon WHERE entry=106777;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Last Travard binding query failed'}}finally{$env:MYSQL_PWD=$previous}
 $expected=@("binding`t106777`tnpc_travar_last","spawn`t362048`t106777`t1539`t7003","phase`t7003`t44`t1092`t9`t0","phase`t7003`t44`t1092`t10`t1","aura`t106777`t215258")
 if(Compare-Object $expected $live){throw 'Native last Travard actor/phase/aura changed'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_wave_lifetime_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed wave lifetime repair missing'}}finally{$env:PYTHONPATH=$previous}
}
