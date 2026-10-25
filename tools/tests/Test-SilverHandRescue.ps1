param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned baseline cannot certify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/silver-hand-rescue"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "0a643f8a:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned rescue baseline missing'}}
[IO.File]::WriteAllText("$out/RescuePolicy.inc",$(if($Baseline){''}else{Block $source 'namespace SilverHandRescue'}))
[IO.File]::WriteAllText("$out/RescueAI.inc",(Block $source 'struct npc_silverhand_eventsAI : ScriptedAI')+';')
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
[IO.File]::WriteAllText("$out/UnitState.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'enum UnitState')+';')
$heal=Block $unit 'int32 Unit::DealHeal('
$last=$heal.IndexOf('    Unit* unit = this;')
if($last -lt 0){throw 'Native heal suffix boundary missing'}
$prefix=$heal.Substring(0,$last).Replace(', SpellInfo const* spellProto','')+"    return gain;`n}"
if($prefix.IndexOf('HealReceived(this, addhealth)') -gt $prefix.IndexOf('HealDone(victim, addhealth)') -or $prefix.IndexOf('HealDone(victim, addhealth)') -gt $prefix.IndexOf('ModifyHealth(int32(addhealth), this)')){throw 'Native heal callback/application order changed'}
[IO.File]::WriteAllText("$out/DealHealPrefix.inc",$prefix)
[IO.File]::WriteAllText("$out/ModifyHealth.inc",(Block $unit 'int64 Unit::ModifyHealth('))
[IO.File]::WriteAllText("$out/LifecycleBridge.inc",(Block (Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw) 'if (i_AI || i_disabledAI)'))
[IO.File]::WriteAllText("$out/OwnerPlayer.inc",(Block $unit 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'))
$functions=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/FunctionClass.inc",(Block (Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw) 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';')
$methods=@('FunctionProcessor::FunctionProcessor()','FunctionProcessor::~FunctionProcessor()','void FunctionProcessor::Update(','void FunctionProcessor::KillAllFunctions()','void FunctionProcessor::AddFunction(','void FunctionProcessor::AddFunctionsFromQueue()','uint64 FunctionProcessor::CalculateTime(','void FunctionProcessor::AddDelayedEvent(')
[IO.File]::WriteAllText("$out/FunctionMethods.inc",(($methods|ForEach-Object{Block $functions $_}) -join "`n"))
$guid=Get-Content "$repo/src/server/game/Entities/Object/ObjectGuid.cpp" -Raw
[IO.File]::WriteAllText("$out/GuidMethods.inc",((@('ObjectGuid::ObjectGuid()','void ObjectGuid::Clear()','bool ObjectGuid::IsEmpty() const')|ForEach-Object{Block $guid $_}) -join "`n"))
$accessor=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.h" -Raw
[IO.File]::WriteAllText("$out/WorldPlayer.inc",(Block $accessor 'static Player* GetObjectInWorld(ObjectGuid'))
[IO.File]::WriteAllText("$out/MapPlayer.inc",(Block $accessor 'template<class T> static T* GetObjectInMap('))
[IO.File]::WriteAllText("$out/AccessorPlayer.inc",(Block (Get-Content "$repo/src/server/game/Globals/ObjectAccessor.cpp" -Raw) 'Player* ObjectAccessor::GetPlayer('))
[IO.File]::WriteAllText("$out/CriteriaTypes.inc",(Block (Get-Content "$repo/src/server/game/DataStores/DBCEnums.h" -Raw) 'enum CriteriaTypes')+';')
[IO.File]::WriteAllText("$out/GroupTypes.inc",(Block (Get-Content "$repo/src/server/game/Achievements/AchievementMgr.cpp" -Raw) 'bool AchievementGlobalMgr::IsGroupCriteriaType('))
$player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$first=$player.IndexOf('    if (!ignoreGroup && sAchievementMgr->IsGroupCriteriaType(type))',$player.IndexOf('void Player::UpdateAchievementCriteria('))
$last=$player.IndexOf('    // Update only individual achievement criteria here, otherwise we may get multiple updates',$first+1)
if($first -lt 0 -or $last -le $first){throw 'Native scenario bridge missing'}
[IO.File]::WriteAllText("$out/ScenarioBridge.inc",$player.Substring($first,$last-$first))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/SilverHandRescueTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native rescue post-heal lifecycle failed'}
if($Baseline){return}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SilverHandAllyCatalog.cpp -ProbeName SilverHandAllyCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native rescue metadata failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv"
foreach($expected in @("CRITERION`t7`t30087`t92`t50913`t2","CRITERION`t7`t30088`t92`t50914`t2","EFFECT`t211698`t0`t6`t4`t127`t0`t1`t0","EFFECT`t211699`t0`t6`t226`t0`t0`t1`t0")){if($native -notcontains $expected){throw "Native rescue criterion/aura changed: $expected"}}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$rows=@("SET SESSION TRANSACTION READ ONLY;SELECT 'binding',entry,ScriptName FROM creature_template WHERE ScriptName='npc_silverhand_events';SELECT 'spawn',guid,id,map,PhaseId,curhealth FROM creature WHERE id IN(106933,106721,106722);SELECT 'addon',entry,auras FROM creature_template_addon WHERE entry IN(106933,106721,106722);SELECT 'phase',pd.phaseId,co.ConditionTypeOrReference,co.ConditionValue1,co.ConditionValue2 FROM phase_definitions pd JOIN conditions co ON co.SourceGroup=pd.zoneId AND co.SourceEntry=pd.entry AND co.SourceTypeOrReferenceId=23 WHERE pd.zoneId=85 AND pd.entry=8;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Rescue binding read failed'}}finally{$env:MYSQL_PWD=$previous}
 $expected=@("binding`t106721`tnpc_silverhand_events","binding`t106722`tnpc_silverhand_events","binding`t106933`tnpc_silverhand_events","spawn`t362032`t106721`t1539`t7001`t1","spawn`t362031`t106722`t1539`t7001`t1","spawn`t362023`t106933`t1539`t7001`t1","spawn`t362028`t106933`t1539`t7001`t1","addon`t106721`t211699","addon`t106722`t211699","addon`t106933`t211698","phase`t7001`t44`t1092`t7")
 if(Compare-Object $expected $rows){throw 'Native four-rescue spawn/phase/aura binding changed'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_rescue_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed rescue post-heal lifecycle missing'}}finally{$env:PYTHONPATH=$previous}
}
