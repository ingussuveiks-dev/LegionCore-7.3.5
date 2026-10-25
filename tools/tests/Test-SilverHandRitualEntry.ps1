param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned baseline cannot certify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/silver-hand-ritual-entry"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "952c2ad5:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned ritual entry baseline missing'}}
[IO.File]::WriteAllText("$out/RitualPolicy.inc",$(if($Baseline){''}else{Block $source 'namespace SilverHandRitualEntry'}))
$actor=Block $source 'struct npc_travar_thirdAI : ScriptedAI'
[IO.File]::WriteAllText("$out/RitualLos.inc",(Block $actor 'void MoveInLineOfSight(').Replace(' override',''))
$guid=Get-Content "$repo/src/server/game/Entities/Object/ObjectGuid.cpp" -Raw
[IO.File]::WriteAllText("$out/GuidLifecycle.inc",((@('ObjectGuid::ObjectGuid()','void ObjectGuid::Clear()','bool ObjectGuid::IsEmpty() const')|ForEach-Object{Block $guid $_}) -join "`n"))
$header=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.h" -Raw
[IO.File]::WriteAllText("$out/WorldPlayer.inc",(Block $header 'static Player* GetObjectInWorld(ObjectGuid'))
[IO.File]::WriteAllText("$out/MapPlayer.inc",(Block $header 'template<class T> static T* GetObjectInMap('))
[IO.File]::WriteAllText("$out/AccessorPlayer.inc",(Block (Get-Content "$repo/src/server/game/Globals/ObjectAccessor.cpp" -Raw) 'Player* ObjectAccessor::GetPlayer('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/SilverHandRitualEntryTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native ritual entry lifecycle failed'}
if($Baseline){return}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SilverHandPickupCatalog.cpp -ProbeName SilverHandPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native ritual metadata failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
foreach($expected in @("CRITERION`t5`t30080`t92`t50896`t0","CRITERION`t5`t30074`t92`t50877`t0","CRITERION`t6`t30086`t92`t50912`t0","CRITERION`t8`t30089`t92`t50915`t0")){if((Get-Content "$out/native.tsv") -notcontains $expected){throw "Native ritual criteria changed: $expected"}}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$live=@("SET SESSION TRANSACTION READ ONLY;SELECT 'binding',entry,ScriptName FROM creature_template WHERE ScriptName='npc_travar_third';SELECT 'spawn',guid,id,map,PhaseId FROM creature WHERE id=106676;SELECT 'phase',pd.phaseId,co.ConditionTypeOrReference,co.ConditionValue1,co.ConditionValue2,co.NegativeCondition,co.ElseGroup FROM phase_definitions pd JOIN conditions co ON co.SourceGroup=pd.zoneId AND co.SourceEntry=pd.entry AND co.SourceTypeOrReferenceId=23 WHERE pd.zoneId=85 AND pd.entry=9;SELECT 'path',id,COUNT(*),MIN(point),MAX(point) FROM waypoint_data_script WHERE id=10993807 GROUP BY id;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Ritual entry binding query failed'}}finally{$env:MYSQL_PWD=$previous}
 $expected=@("binding`t106676`tnpc_travar_third","spawn`t362006`t106676`t1539`t7002","path`t10993807`t22`t1`t22")
 foreach($step in 5..8){$expected+="phase`t7002`t44`t1092`t$step`t0`t$($step-5)"}
 if(Compare-Object $expected $live){throw 'Native ritual phase/actor/path changed'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_ritual_entry_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed ritual entry guard missing'}}finally{$env:PYTHONPATH=$previous}
}
