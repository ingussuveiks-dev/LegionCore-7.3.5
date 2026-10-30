param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned baseline cannot certify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/silver-hand-aberration-credit"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp'
$source=Get-Content "$repo/$path" -Raw
$pinned=(& git -C $repo show "fc0bc298:$path") -join "`n"
if($LASTEXITCODE){throw 'Pinned aberration baseline missing'}
if($Baseline){$source=$pinned}
$actor=Block $source 'struct npc_horrific_aberrationAI : ScriptedAI'
[IO.File]::WriteAllText("$out/CreditPolicy.inc",$(if($Baseline){''}else{Block $source 'namespace SilverHandAberration'}))
foreach($method in @('Reset','JustDied')){[IO.File]::WriteAllText("$out/$method.inc",(Block $actor "void $method(").Replace(' override',''))}
if(!$Baseline){
 & "$PSScriptRoot/Test-SilverHandAberrationTransition.ps1" -ScopeOnly
 $transition=(& git -C $repo show "89231609d:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned transition scope missing'}
 $beforeTransition=Block $transition 'struct npc_horrific_aberrationAI : ScriptedAI'
 foreach($pair in @(@('void DamageTakenAfterScaling(','void DamageTaken('),@('void UpdateAI(','void UpdateAI('))){$actor=$actor.Replace((Block $actor $pair[0]),(Block $beforeTransition $pair[1]))}
 $actor=$actor.Replace((Block $actor 'void UpdateLifecycle('),'')
 foreach($field in @('uint32 _outroTimer = 0;','bool _outroFinished = false;','uint64 _outroGeneration = 0;')){$actor=$actor.Replace($field,'')}
 $actor=$actor.Replace('bool outroDone = false;','bool outroDone;')
 $original=Block $pinned 'struct npc_horrific_aberrationAI : ScriptedAI'
 foreach($method in @('void Reset(','void JustDied(')){$original=$original.Replace((Block $original $method),'');$actor=$actor.Replace((Block $actor $method),'')}
 $actor=$actor.Replace('bool _deathCreditDone = false;','')
 if(($original -replace '\s','') -cne ($actor -replace '\s','')){throw 'Unrelated aberration combat/choreography changed'}
}
[IO.File]::WriteAllText("$out/OwnerPlayer.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw) 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'))
[IO.File]::WriteAllText("$out/CriteriaTypes.inc",(Block (Get-Content "$repo/src/server/game/DataStores/DBCEnums.h" -Raw) 'enum CriteriaTypes')+';')
[IO.File]::WriteAllText("$out/GroupTypes.inc",(Block (Get-Content "$repo/src/server/game/Achievements/AchievementMgr.cpp" -Raw) 'bool AchievementGlobalMgr::IsGroupCriteriaType('))
$player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$first=$player.IndexOf('    if (!ignoreGroup && sAchievementMgr->IsGroupCriteriaType(type))',$player.IndexOf('void Player::UpdateAchievementCriteria('))
$last=$player.IndexOf('    // Update only individual achievement criteria here, otherwise we may get multiple updates',$first+1)
if($first -lt 0 -or $last -le $first){throw 'Native scenario bridge missing'}
[IO.File]::WriteAllText("$out/ScenarioBridge.inc",$player.Substring($first,$last-$first))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/SilverHandAberrationCreditTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native aberration death credit failed'}
if($Baseline){return}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SilverHandPickupCatalog.cpp -ProbeName SilverHandPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native aberration criteria failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
foreach($expected in @("CRITERION`t5`t30080`t92`t50896`t0","CRITERION`t5`t30074`t92`t50877`t0","CRITERION`t7`t30087`t92`t50913`t2","CRITERION`t7`t30088`t92`t50914`t2")){if((Get-Content "$out/native.tsv") -notcontains $expected){throw "Native aberration/ritual criteria changed: $expected"}}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$rows=@("SET SESSION TRANSACTION READ ONLY;SELECT 'binding',entry,ScriptName FROM creature_template WHERE ScriptName='npc_horrific_aberration';SELECT 'spawn',guid,id,map,PhaseId FROM creature WHERE id=106669;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Aberration binding read failed'}}finally{$env:MYSQL_PWD=$previous}
 if(Compare-Object @("binding`t106669`tnpc_horrific_aberration","spawn`t361979`t106669`t1539`t7002") $rows){throw 'Aberration live binding changed'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_aberration_credit_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed aberration death credit missing'}}finally{$env:PYTHONPATH=$previous}
}
