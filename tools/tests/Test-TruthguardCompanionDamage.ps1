param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$OutputDirectory='')
$ErrorActionPreference='Stop'
if($VerifyReleaseBinary -and ($Baseline -or $ScopeOnly)){throw 'Only repaired code certifies Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth -and $last -lt $source.Length){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};if($depth){throw "Unclosed $start"};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/shields_rest.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'')
$pinned=(& git -C $repo show "a148a34b5:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned companion baseline missing'}
$original=Block (Block $pinned 'class npc_sr_orik_cato_script :') 'void DamageTaken('
$expected=$original.Replace('void DamageTaken(','void DamageTakenAfterScaling(').Replace('DamageEffectType dmgType','DamageEffectType /*dmgType*/').Replace('me->GetHealth()','me->GetHealth(attacker)').Replace('                attacker->getThreatManager().modifyThreatPercent(me, -90);',"                if (attacker)`n                    attacker->getThreatManager().modifyThreatPercent(me, -90);")
if(!$Baseline){
 $actual=Block (Block $source 'class npc_sr_orik_cato_script :') 'void DamageTakenAfterScaling('
 if($actual -cne $expected -or $source.Replace($actual,$original).TrimEnd("`n") -cne $pinned.TrimEnd("`n")){throw 'Unrelated Shield Rest method, timers, spells, movement or actor changed'}
}
if($ScopeOnly){'PASS exact companion damage-hook/health/null-attacker scope; whole remaining Shield Rest source unchanged';return}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/truthguard-companion-damage"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
[IO.File]::WriteAllText("$out/Damage.inc",$(if($Baseline){$original}else{$actual}))
$core=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$first=$core.IndexOf('    damage = uint32(std::min<double>(damage / victim->GetHealthMultiplierForTarget(this), std::numeric_limits<uint32>::max()));')
if($first -lt 0){throw 'Native final scaling missing'}
$bridge=Block $core.Substring($first) '    if (Creature* creature = victim->ToCreature())'
if($core.IndexOf('    if (health <= damage)',$first) -lt $first+$bridge.Length){throw 'Native final-hook/death order changed'}
[IO.File]::WriteAllText("$out/ScaledHook.inc",'    damage = uint32(std::min<double>(damage / victim->GetHealthMultiplierForTarget(this), std::numeric_limits<uint32>::max()));'+$bridge)
[IO.File]::WriteAllText("$out/Health.inc",(Block $core 'uint64 Unit::GetHealth(Unit* victim) const'))
[IO.File]::WriteAllText("$out/Multiplier.inc",(Block (Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw) 'float Creature::GetHealthMultiplierForTarget('))
$kill=Block $core 'void Unit::Kill('
if(!$kill.Contains('victim->setDeathState(JUST_DIED);') -or $kill.Contains('DamageTakenAfterScaling')){throw 'Scripted Kill no longer bypasses ordinary damage policy'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/TruthguardCompanionDamageTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw "Native companion damage failed: $LASTEXITCODE"}
if($VerifyReleaseBinary){
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TruthguardCompanionCatalog.cpp -ProbeName TruthguardCompanionCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native companion catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 foreach($row in @("EFFECT`t210036`t0`t6`t428`t1`t209945`t1`t0","EFFECT`t210043`t0`t6`t428`t1`t209953`t1`t0","CRITERION`t1`t29808`t92`t50500`t0")){if((Get-Content "$out/native.tsv") -notcontains $row){throw "Native companion data changed: $row"}}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'binding',entry,AIName,ScriptName FROM creature_template WHERE ScriptName='npc_sr_orik_cato_script' ORDER BY entry;
SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry IN(105911,105912) ORDER BY Entry;
SELECT 'spawn',COUNT(*) FROM creature WHERE id IN(105911,105912);
SELECT 'smart',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid IN(105911,105912);
SELECT 'scenario',ScenarioID,MapID,Team,Class FROM scenario_data WHERE MapID=1495 ORDER BY ScenarioID;
SELECT 'complete','truthguard-companion-readback';
'@
 [IO.File]::WriteAllText("$out/live-read.sql",$query)
 $previous=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$connection[3]
  $arguments=@("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names')
  $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $arguments -InputFile "$out/live-read.sql" -OutputFile "$out/live.tsv" -ErrorFile "$out/live.stderr"
  if($code -or (Get-Item "$out/live.stderr").Length){throw 'Full-EOF companion readback failed'}
 }finally{$env:MYSQL_PWD=$previous}
 $expected=@("binding`t105911`t`tnpc_sr_orik_cato_script","binding`t105912`t`tnpc_sr_orik_cato_script","scaling`t105911`t98`t110","scaling`t105912`t98`t110","spawn`t0","smart`t0","scenario`t909`t1495`t0`t1","scenario`t1068`t1495`t0`t3","scenario`t1082`t1495`t0`t2","complete`ttruthguard-companion-readback")
 if(Compare-Object $expected @(Get-Content "$out/live.tsv")){throw 'Companion binding/scaling/shared-map readback changed'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_truthguard_companion_damage_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical companion final-damage PE failed'}}finally{$env:PYTHONPATH=$previous}
}
