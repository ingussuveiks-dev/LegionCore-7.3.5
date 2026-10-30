param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned baseline cannot verify installed code'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/truthguard-surrender"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/shields_rest.cpp'
$source=Get-Content "$repo/$path" -Raw
$pinned=(& git -C $repo show "b81a0ede:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Yrgrim source missing'}
if($Baseline){$source=$pinned}
if(!$Baseline){
 $original=Block $pinned '    struct npc_sr_yrgrim_the_truthseekerAI'
 $expected=$original.Replace('void DamageTaken(Unit* /*attacker*/,','void DamageTakenAfterScaling(Unit* attacker,').Replace('damage >= me->GetHealth()','damage >= me->GetHealth(attacker)')
 if((Block $source '    struct npc_sr_yrgrim_the_truthseekerAI').Replace("`r",'') -cne $expected.Replace("`r",'')){throw 'Yrgrim reset, movement, stages, summon, combat or surrender side effects changed beyond the audited hook/health correction'}
}
$source=$source.Substring($source.IndexOf('    struct npc_sr_yrgrim_the_truthseekerAI'))
[IO.File]::WriteAllText("$out/Surrender.inc",(Block $source $(if($Baseline){'void DamageTaken('}else{'void DamageTakenAfterScaling('})))
$core=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$first=$core.IndexOf('    damage = uint32(std::min<double>(damage / victim->GetHealthMultiplierForTarget(this), std::numeric_limits<uint32>::max()));')
if($first -lt 0){throw 'Native final scaling missing'}
$bridge=Block $core.Substring($first) '    if (Creature* creature = victim->ToCreature())'
if($core.IndexOf('    if (health <= damage)',$first) -lt $first+$bridge.Length){throw 'Native final protection/death order changed'}
[IO.File]::WriteAllText("$out/ScaledHook.inc",'    damage = uint32(std::min<double>(damage / victim->GetHealthMultiplierForTarget(this), std::numeric_limits<uint32>::max()));'+$bridge)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$out`" `"$PSScriptRoot/TruthguardSurrenderTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Extracted Yrgrim surrender regression failed'}
if($Baseline){return}
$previousConfig=$env:LEGION_DIALOGUE_CONFIG
try{
 $env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
 & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TruthguardSurrenderCatalog.cpp -ProbeName TruthguardSurrenderCatalog -OutputFile "$out/native.tsv"
}finally{$env:LEGION_DIALOGUE_CONFIG=$previousConfig}
$native=Get-Content "$out/native.tsv"
foreach($expected in @("STEP`t3`t2392`t47521`t0`tShrine of the Truthguard","CRITERION`t3`t29842`t92`t50581`t0","STEP`t4`t2394`t47523`t0`tYrgrim's Challenge","CRITERION`t4`t30142`t92`t50598`t0","STEP`t5`t2395`t47525`t0`tThe Truthguard","CRITERION`t5`t29844`t92`t50545`t0")){
 if($native -notcontains $expected){throw "Matching native scenario1082 differs: $expected"}
}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'binding',entry,AIName,ScriptName FROM creature_template WHERE ScriptName='npc_sr_yrgrim_the_truthseeker' ORDER BY entry;
SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry IN(105695,106202) ORDER BY Entry;
SELECT 'scenario',ScenarioID,MapID,Team,Class FROM scenario_data WHERE MapID=1495 ORDER BY ScenarioID;
'@
 $password=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$rows=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Yrgrim live readback failed'}}finally{$env:MYSQL_PWD=$password}
 foreach($expected in @("binding`t105695`t`tnpc_sr_yrgrim_the_truthseeker","binding`t106202`t`tnpc_sr_yrgrim_the_truthseeker","scaling`t105695`t98`t110","scaling`t106202`t98`t110","scenario`t909`t1495`t0`t1","scenario`t1068`t1495`t0`t3","scenario`t1082`t1495`t0`t2")){
  if($rows -notcontains $expected){throw "Live Yrgrim/shared map differs: $expected"}
 }
 if(@($rows|Where-Object {$_ -like 'binding*'}).Count -ne 2){throw 'Additional shared Yrgrim binding needs review'}
 $rows|Set-Content -Encoding utf8 "$out/live.tsv"
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_truthguard_surrender_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed Yrgrim surrender missing'}}finally{$env:PYTHONPATH=$previous}
}
