param([switch]$Baseline,[switch]$HealBaseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if(($Baseline -and $HealBaseline) -or (($Baseline -or $HealBaseline) -and $VerifyReleaseBinary)){throw 'Pinned baseline modes are mutually exclusive and cannot verify current Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/silver-hand-ally-damage"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
function ArrivalCase([string]$movement){$first=$movement.IndexOf('case 1:');$last=$movement.IndexOf('case 2:',$first);if($first -lt 0 -or $last -le $first){throw 'POINT1 block missing'};$movement.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp'
$source=Get-Content "$repo/$path" -Raw
$pinned=(& git -C $repo show "d49e77d2:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Silver Hand source missing'}
$healPinned=(& git -C $repo show "877ad851:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned companion healing source missing'}
$old="void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType dmgType) override`n        {`n            if (damage >= me->GetHealth())"
$new=$old.Replace('void DamageTaken(','void DamageTakenAfterScaling(').Replace('me->GetHealth()','me->GetHealth(attacker)')
if([regex]::Matches($pinned,[regex]::Escape($old)).Count -ne 4){throw 'Pinned four ally guards differ'}
foreach($script in @('npc_travar_first','npc_travar_second','npc_travar_third','npc_efrin_and_duval')){
 if(!$Baseline -and !$HealBaseline){
  $actual=(Block $source "class $script : public CreatureScript").Replace("`r",'')
  $beforeHeal=Block $healPinned "class $script : public CreatureScript"
  $normalized=$actual.Replace("        bool _pendingHealRecovery = false;`n",'').Replace("                    _pendingHealRecovery = false;`n",'').Replace("            _pendingHealRecovery = false;`n",'')
  $normalized=$normalized.Replace("        "+(Block $normalized 'void UpdateLifecycle(')+"`n`n",'')
  $normalized=$normalized.Replace((Block $normalized 'void HealReceived('),(Block $beforeHeal 'void HealReceived('))
  if($normalized -cne $beforeHeal){throw 'Companion changed beyond pending state, reset/wound invalidation and post-heal lifecycle'}
  $actual=$normalized
  $expected=(Block $pinned "class $script : public CreatureScript").Replace($old,$new)
  if($script -eq 'npc_travar_first'){
   $actual=$actual.Replace((Block $actual 'void MoveInLineOfSight('),(Block $expected 'void MoveInLineOfSight('))
   $actual=$actual.Replace("        "+(Block $actual 'void sGossipSelect(')+"`n`n",'')
   $actual=$actual.Replace("        "+(Block $actual 'void CompleteEntranceArrival(')+"`n`n",'').Replace("        ObjectGuid _arrivalPlayerGuid;`n",'')
   $movement=Block $actual 'void MovementInform('
   $actual=$actual.Replace($movement,$movement.Replace((ArrivalCase $movement),(ArrivalCase (Block $expected 'void MovementInform('))))
   $expected=$expected.Replace("    "+(Block $expected 'bool OnGossipSelect(')+"`n`n",'')
  }
  elseif($script -eq 'npc_travar_second'){
   $actual=$actual.Replace("        "+(Block $actual 'void sGossipSelect(')+"`n`n",'')
   $actual=$actual.Replace("        "+(Block $actual 'void CompleteProcession(')+"`n`n",'').Replace("        ObjectGuid _processionPlayerGuid;`n",'')
   $actual=$actual.Replace("                if (id == 14)`n                    CompleteProcession();",(Block $expected 'if (id == 14)').Insert(0,'                '))
   $expected=$expected.Replace("    "+(Block $expected 'bool OnGossipSelect(')+"`n`n",'')
  }
  elseif($script -eq 'npc_travar_third'){
   $actual=$actual.Replace((Block $actual 'void MoveInLineOfSight('),(Block $expected 'void MoveInLineOfSight('))
  }
  if($actual -cne $expected){throw 'Silver Hand ally class changed beyond its hook/health correction or separately tested first entrance/second procession/third entry'}
 }
}
if($Baseline){$source=$pinned}
if($HealBaseline){$source=$healPinned}
[IO.File]::WriteAllText("$out/Recovery.inc",$(if($Baseline -or $HealBaseline){''}else{Block $source 'namespace SilverHandCompanion'}))
foreach($pair in @(@('First','npc_travar_firstAI'),@('Second','npc_travar_secondAI'),@('Third','npc_travar_thirdAI'),@('Allies','npc_efrin_and_duvalAI'))){
 $actor=Block $source "struct $($pair[1]) : ScriptedAI"
 [IO.File]::WriteAllText("$out/$($pair[0])Damage.inc",(Block $actor $(if($Baseline){'void DamageTaken('}else{'void DamageTakenAfterScaling('})))
 [IO.File]::WriteAllText("$out/$($pair[0])Heal.inc",(Block $actor 'void HealReceived('))
 [IO.File]::WriteAllText("$out/$($pair[0])Lifecycle.inc",$(if($Baseline -or $HealBaseline){''}else{Block $actor 'void UpdateLifecycle('}))
 [IO.File]::WriteAllText("$out/$($pair[0])Reset.inc",(Block $actor 'void Reset('))
}
$core=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$heal=Block $core 'int32 Unit::DealHeal('
$last=$heal.IndexOf('    Unit* unit = this;')
if($last -lt 0){throw 'Native heal suffix boundary missing'}
$prefix=$heal.Substring(0,$last).Replace(', SpellInfo const* spellProto','')+"    return gain;`n}"
if($prefix.IndexOf('HealReceived(this, addhealth)') -gt $prefix.IndexOf('HealDone(victim, addhealth)') -or $prefix.IndexOf('HealDone(victim, addhealth)') -gt $prefix.IndexOf('ModifyHealth(int32(addhealth), this)')){throw 'Native heal callback/application order changed'}
[IO.File]::WriteAllText("$out/DealHealPrefix.inc",$prefix)
[IO.File]::WriteAllText("$out/ModifyHealth.inc",(Block $core 'int64 Unit::ModifyHealth('))
[IO.File]::WriteAllText("$out/LifecycleBridge.inc",(Block (Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw) 'if (i_AI || i_disabledAI)'))
$first=$core.IndexOf('    damage /= victim->GetHealthMultiplierForTarget(this);')
$bridge=Block $core.Substring($first) '    if (Creature* creature = victim->ToCreature())'
if($first -lt 0 -or $core.IndexOf('    if (health <= damage)',$first) -lt $first+$bridge.Length){throw 'Native scaling/protection/death order changed'}
[IO.File]::WriteAllText("$out/ScaledHook.inc",'    damage /= victim->GetHealthMultiplierForTarget(this);'+$bridge)
[IO.File]::WriteAllText("$out/Health.inc",(Block $core 'uint64 Unit::GetHealth(Unit* victim) const'))
$kill=Block $core 'void Unit::Kill('
if(!$kill.Contains('victim->setDeathState(JUST_DIED);') -or $kill.Contains('DamageTakenAfterScaling')){throw 'Direct scripted Kill no longer bypasses the ordinary damage guard'}
[IO.File]::WriteAllText("$out/Multiplier.inc",(Block (Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw) 'float Creature::GetHealthMultiplierForTarget('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DSILVER_HAND_ALLY_BASELINE'}elseif($HealBaseline){'/DSILVER_HAND_HEAL_BASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$out`" `"$PSScriptRoot/SilverHandAllyDamageTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Silver Hand ally protection failed'}
if($Baseline -or $HealBaseline){return}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SilverHandAllyCatalog.cpp -ProbeName SilverHandAllyCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native Silver Hand metadata failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv"
foreach($expected in @("CRITERION`t2`t30046`t92`t50850`t0","CRITERION`t5`t30080`t92`t50896`t0","CRITERION`t7`t30087`t92`t50913`t2","CRITERION`t9`t30110`t92`t51838`t1","CRITERION`t10`t30111`t92`t50966`t0","EFFECT`t210985`t0`t6`t226`t0`t0`t1`t0")){if($native -notcontains $expected){throw "Silver Hand native metadata differs: $expected"}}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'binding',entry,AIName,ScriptName FROM creature_template WHERE ScriptName IN('npc_travar_first','npc_travar_second','npc_travar_third','npc_efrin_and_duval') ORDER BY entry;
SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry IN(106429,106639,106676,106370,106371,106774) ORDER BY Entry;
SELECT 'scenario',ScenarioID,MapID,Class FROM scenario_data WHERE MapID=1539 ORDER BY ScenarioID;
SELECT 'smart',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid IN(106429,106639,106676,106370,106371,106774);
'@
 $password=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$rows=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Silver Hand binding read failed'}}finally{$env:MYSQL_PWD=$password}
 $expected=@("binding`t106370`t`tnpc_efrin_and_duval","binding`t106371`t`tnpc_efrin_and_duval","binding`t106429`t`tnpc_travar_first","binding`t106639`t`tnpc_travar_second","binding`t106676`t`tnpc_travar_third","binding`t106774`t`tnpc_efrin_and_duval","scenario`t991`t1539`t5","scenario`t1037`t1539`t1","scenario`t1092`t1539`t2","smart`t0")
 foreach($identity in @(106370,106371,106429,106639,106676,106774)){$expected+="scaling`t$identity`t98`t110"}
 if(Compare-Object $expected $rows){throw 'Silver Hand native binding/scaling/shared-map contract changed'}
 $rows|Set-Content -Encoding utf8 "$out/live.tsv"
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_ally_damage_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed Silver Hand ally protection failed'}}finally{$env:PYTHONPATH=$previous}
}
