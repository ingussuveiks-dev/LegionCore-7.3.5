param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot certify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/silver-hand-ritual-dispatch"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/instance_silver_hand.cpp'
$source=Get-Content "$repo/$path" -Raw
$pinned=(& git -C $repo show "e01a2c64:$path") -join "`n"
if($LASTEXITCODE){throw 'Pinned instance baseline unavailable'}
if($Baseline){$source=$pinned}else{
 $original=Block $pinned 'void onScenarionNextStep('
 $current=Block $source 'void onScenarionNextStep('
 $before='if (Creature* travar = player->FindNearestCreature(106676, 70.0f, true)) travar->AI()->DoAction(true); break;'
 $after='if (Creature* travar = player->FindNearestCreature(106676, 70.0f, true)) { travar->AI()->DoAction(true); break; }'
 if(($current -replace '\s','') -cne (($original -replace '\s','').Replace(($before -replace '\s',''),($after -replace '\s','')))){throw 'Unrelated instance callback behavior changed'}
 if(($source.Replace($current,$original) -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Unrelated instance script behavior changed'}
}
[IO.File]::WriteAllText("$out/Dispatch.inc",(Block $source 'void onScenarionNextStep(').Replace(' override',''))
$actorSource=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp" -Raw
$actor=Block $actorSource 'struct npc_travar_thirdAI : ScriptedAI'
[IO.File]::WriteAllText("$out/Action.inc",(Block $actor 'void DoAction(').Replace(' override',''))
[IO.File]::WriteAllText("$out/Update.inc",(Block $actor 'void UpdateAI(').Replace(' override',''))
[IO.File]::WriteAllText("$out/Movement.inc",(Block $actor 'void MovementInform(').Replace(' override',''))
$functions=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/FunctionClass.inc",(Block (Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw) 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';')
$methods=@('FunctionProcessor::FunctionProcessor()','FunctionProcessor::~FunctionProcessor()','void FunctionProcessor::Update(','void FunctionProcessor::KillAllFunctions()','void FunctionProcessor::AddFunction(','void FunctionProcessor::AddFunctionsFromQueue()','uint64 FunctionProcessor::CalculateTime(','void FunctionProcessor::AddDelayedEvent(')
[IO.File]::WriteAllText("$out/FunctionMethods.inc",(($methods|ForEach-Object{Block $functions $_}) -join "`n"))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/SilverHandRitualDispatchTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native ritual dispatch regression failed'}
if($Baseline){return}
& "$PSScriptRoot/Test-SilverHandRitualEntry.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary
if(!$?){throw 'Existing ritual admission/lifetime/data checks failed'}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$live=@("SET SESSION TRANSACTION READ ONLY;SELECT map,script FROM instance_template WHERE map=1539;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Instance binding query failed'}}finally{$env:MYSQL_PWD=$previous}
 if($live.Count -ne 1 -or $live[0] -ne "1539`tinstance_silver_hand"){throw 'Instance binding changed'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_ritual_dispatch_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed ritual dispatch repair missing'}}finally{$env:PYTHONPATH=$previous}
}
