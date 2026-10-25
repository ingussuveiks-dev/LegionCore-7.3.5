param([switch]$Baseline,[switch]$FinalBaseline,[switch]$ActionBaseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if(@($Baseline,$FinalBaseline,$ActionBaseline|Where-Object {$_}).Count -gt 1 -or (($Baseline -or $FinalBaseline -or $ActionBaseline) -and $VerifyReleaseBinary)){throw 'Pinned baselines are mutually exclusive and cannot certify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/silver-hand-ritual-dispatch"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/instance_silver_hand.cpp'
$source=Get-Content "$repo/$path" -Raw
$pinned=(& git -C $repo show "e01a2c64:$path") -join "`n"
if($LASTEXITCODE){throw 'Pinned instance baseline unavailable'}
if($FinalBaseline){$source=(& git -C $repo show "7680556f:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned final-stage baseline unavailable'}}
elseif($Baseline){$source=$pinned}else{
 $original=Block $pinned 'void onScenarionNextStep('
 $current=Block $source 'void onScenarionNextStep('
 $before='if (Creature* travar = player->FindNearestCreature(106676, 70.0f, true)) travar->AI()->DoAction(true); break;'
 $after='if (Creature* travar = player->FindNearestCreature(106676, 70.0f, true)) { travar->AI()->DoAction(true); break; }'
 $oldFinal=Block $original 'if (newStep == 10)'
 $newFinal=Block $current 'if (newStep == 10)'
 if(($current.Replace($newFinal,$oldFinal) -replace '\s','') -cne (($original -replace '\s','').Replace(($before -replace '\s',''),($after -replace '\s','')))){throw 'Unrelated step8 callback behavior changed'}
 if(($source.Replace($current,$original) -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Unrelated instance script behavior changed'}
}
[IO.File]::WriteAllText("$out/Dispatch.inc",(Block $source 'void onScenarionNextStep(').Replace(' override',''))
$actorSource=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp" -Raw
if($ActionBaseline){$actorSource=(& git -C $repo show '41ce375b:src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned ritual action baseline unavailable'}}
$actor=Block $actorSource 'struct npc_travar_thirdAI : ScriptedAI'
[IO.File]::WriteAllText("$out/ActionPolicy.inc",$(if($ActionBaseline){''}else{Block $actorSource 'bool CanAdvance(Creature* actor)'}))
[IO.File]::WriteAllText("$out/Action.inc",(Block $actor 'void DoAction(').Replace(' override',''))
[IO.File]::WriteAllText("$out/Reset.inc",(Block $actor 'void Reset(').Replace(' override',''))
[IO.File]::WriteAllText("$out/Update.inc",(Block $actor 'void UpdateAI(').Replace(' override',''))
[IO.File]::WriteAllText("$out/Movement.inc",(Block $actor 'void MovementInform(').Replace(' override',''))
$functions=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/FunctionClass.inc",(Block (Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw) 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';')
$methods=@('FunctionProcessor::FunctionProcessor()','FunctionProcessor::~FunctionProcessor()','void FunctionProcessor::Update(','void FunctionProcessor::KillAllFunctions()','void FunctionProcessor::AddFunction(','void FunctionProcessor::AddFunctionsFromQueue()','uint64 FunctionProcessor::CalculateTime(','void FunctionProcessor::AddDelayedEvent(')
[IO.File]::WriteAllText("$out/FunctionMethods.inc",(($methods|ForEach-Object{Block $functions $_}) -join "`n"))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}elseif($FinalBaseline){'/DFINAL_BASELINE'}elseif($ActionBaseline){'/DACTION_BASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/SilverHandRitualDispatchTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native ritual dispatch regression failed'}
if($Baseline -or $FinalBaseline -or $ActionBaseline){return}
& "$PSScriptRoot/Test-SilverHandRitualEntry.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary
if(!$?){throw 'Existing ritual admission/lifetime/data checks failed'}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SilverHandFinalCatalog.cpp -ProbeName SilverHandFinalCatalog -OutputFile "$out/final-native.tsv" *> "$out/final-catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native final spell metadata failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$expected=@("SPELL`t215258`tBringing the Light...","SPELL`t215260`tBlinding Light","EFFECT`t215258`t0`t6`t226`t0`t1`t0","EFFECT`t215260`t0`t6`t60`t0`t22`t7","EFFECT`t215260`t1`t6`t5`t0`t22`t7","EFFECT`t215260`t2`t61`t0`t0`t1`t0","MISC`t215258`t25`t1","MISC`t215260`t21`t1","VISUAL`t215258`t55978`t1`t0`t0","VISUAL`t215260`t57625`t1`t0`t0")
if(Compare-Object $expected (Get-Content "$out/final-native.tsv")){throw 'Native final effects/visual links changed'}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$live=@("SET SESSION TRANSACTION READ ONLY;SELECT map,script FROM instance_template WHERE map=1539;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Instance binding query failed'}}finally{$env:MYSQL_PWD=$previous}
 if($live.Count -ne 1 -or $live[0] -ne "1539`tinstance_silver_hand"){throw 'Instance binding changed'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_ritual_action_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed ritual action/final dispatch repair missing'}}finally{$env:PYTHONPATH=$previous}
}
