param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$OutputDirectory='')
$ErrorActionPreference='Stop'
if($VerifyReleaseBinary -and ($Baseline -or $ScopeOnly)){throw 'Only repaired native checks certify Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth -and $last -lt $source.Length){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};if($depth){throw "Unclosed $start"};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'')
if(!$Baseline -and $source.Contains('namespace SharasdalCompanion')){
 & "$PSScriptRoot/Test-SharasdalCompanionHealing.ps1" -ScopeOnly
 # The healing host compiles the new pending-clear write in the real damage
 # methods. This retained matrix isolates their unchanged wound policy.
 $source=(& git -C $repo show "c3c3ce355:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-heal wound policy missing'}
}
$pinned=(& git -C $repo show "32bfe713f:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Sharasdal source missing'}
$pairs=@(@('Erunak','npc_erunak_stonespeaker'),@('Grash','npc_grash'),@('Adel','npc_adel_waterspeaker'))
$normalized=$source;$handlers=@{}
foreach($pair in $pairs){
 $old=Block (Block $pinned "class $($pair[1]) :") 'void DamageTaken('
 $expected=$old.Replace('void DamageTaken(','void DamageTakenAfterScaling(').Replace('DamageEffectType dmgType','DamageEffectType /*dmgType*/').Replace('me->GetHealth()','me->GetHealth(attacker)')
 if(!$Baseline){$actual=Block (Block $source "class $($pair[1]) :") 'void DamageTakenAfterScaling(';if($actual -cne $expected){throw 'Wound aura/speech/stun/latch changed'};$normalized=$normalized.Replace($actual,$old)}
 $handlers[$pair[0]]=if($Baseline){$old}else{$actual}
}
if(!$Baseline -and $normalized.TrimEnd("`n") -cne $pinned.TrimEnd("`n")){throw 'Unrelated heal, story, click, timer, reset, boss, loot or instance behavior changed'}
if($ScopeOnly){'PASS exact three final-damage/target-health changes only; whole remaining Sharasdal source retained';return}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-companion-damage"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
foreach($name in $handlers.Keys){[IO.File]::WriteAllText("$out/${name}Damage.inc",$handlers[$name])}
$core=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$prefix='    damage = uint32(std::min<double>(damage / victim->GetHealthMultiplierForTarget(this), std::numeric_limits<uint32>::max()));'
$first=$core.IndexOf($prefix);if($first -lt 0){throw 'Native bounded damage missing'}
$bridge=Block $core.Substring($first) '    if (Creature* creature = victim->ToCreature())'
if($core.IndexOf('    if (health <= damage)',$first) -lt $first+$bridge.Length){throw 'Native policy/death order changed'}
[IO.File]::WriteAllText("$out/ScaledHook.inc",$prefix+$bridge)
[IO.File]::WriteAllText("$out/Health.inc",(Block $core 'uint64 Unit::GetHealth(Unit* victim) const'))
[IO.File]::WriteAllText("$out/Multiplier.inc",(Block (Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw) 'float Creature::GetHealthMultiplierForTarget('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SharasdalCompanionDamageTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw "Native Sharasdal wound regression failed: $LASTEXITCODE"}
if($VerifyReleaseBinary){
 $selector=Get-Content "$repo/src/server/game/AI/CreatureAISelector.cpp" -Raw
 $scriptAt=$selector.IndexOf('sScriptMgr->GetCreatureAI(creature)');$nameAt=$selector.IndexOf('creature->GetAIName()')
 if($scriptAt -lt 0 -or $nameAt -lt 0 -or $scriptAt -gt $nameAt){throw 'Review native ScriptName before AIName priority'}
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SharasdalCompanionCatalog.cpp -ProbeName SharasdalCompanionCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native Sharasdal catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 foreach($row in @("EFFECT`t208093`t0`t6`t226`t0`t0`t1`t0","STEP`t0`t2282`t47120`t0`tBig Help","CRITERION`t0`t29658`t92`t49815`t1","CRITERION`t0`t29688`t92`t49900`t1","CRITERION`t0`t29763`t92`t50161`t1","CRITERION`t1`t29659`t92`t49816`t1","CRITERION`t1`t29695`t92`t49957`t1")){if((Get-Content "$out/native.tsv") -notcontains $row){throw "Native Sharasdal context changed: $row"}}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $query="SET SESSION TRANSACTION READ ONLY;SELECT 'binding',entry,AIName,ScriptName FROM creature_template WHERE ScriptName IN('npc_erunak_stonespeaker','npc_grash','npc_adel_waterspeaker') ORDER BY entry;SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry IN(102826,104853,105298) ORDER BY Entry;SELECT 'spawn',guid,id,map,PhaseId FROM creature WHERE id IN(102826,104853,105298) ORDER BY id,map,guid;SELECT 'scenario',ScenarioID,MapID,Team,Class FROM scenario_data WHERE MapID=1600;SELECT 'complete','sharasdal-readback';"
 [IO.File]::WriteAllText("$out/live-read.sql",$query);$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$arguments=@("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names');$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $arguments -InputFile "$out/live-read.sql" -OutputFile "$out/live.tsv" -ErrorFile "$out/live.stderr";if($code -or (Get-Item "$out/live.stderr").Length){throw 'Sharasdal full-EOF readback failed'}}finally{$env:MYSQL_PWD=$previous}
 $expected=@("binding`t102826`t`tnpc_erunak_stonespeaker","binding`t104853`t`tnpc_adel_waterspeaker","binding`t105298`tSmartAI`tnpc_grash","scaling`t102826`t98`t110","scaling`t104853`t98`t110","scaling`t105298`t98`t110","spawn`t362063`t102826`t1600`t6120","spawn`t362093`t102826`t1600`t6120","spawn`t362078`t104853`t1600`t6120","spawn`t362094`t104853`t1600`t6120","spawn`t362066`t105298`t1600`t6120","spawn`t362092`t105298`t1600`t6120","scenario`t1066`t1600`t0`t7","complete`tsharasdal-readback")
 if(Compare-Object $expected @(Get-Content "$out/live.tsv")){throw 'Sharasdal binding/scaling/spawn/shared context changed'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sharasdal_companion_damage_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Sharasdal damage proof failed'}}finally{$env:PYTHONPATH=$previous}
}
