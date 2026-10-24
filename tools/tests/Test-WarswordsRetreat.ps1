param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot certify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-retreat"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Warrior/Warswords/Warswords.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "1e36ad79:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned retreat baseline unavailable'}}
$first=$source.IndexOf('struct npc_vigfus_bladewindAI :');$last=$source.IndexOf('class npc_vigfus_bladewind_last :',$first)
if($first -lt 0 -or $last -le $first){throw 'Missing distinct first boss'}
$source=$source.Substring($first,$last-$first)
foreach($name in @('Reset','DamageTaken','MovementInform')){[IO.File]::WriteAllText("$out/$name.inc",(Block $source "void $name("))}
$post=if($Baseline){''}else{Block $source 'void DamageTakenAfterScaling('}
[IO.File]::WriteAllText("$out/AfterScaling.inc",$post)
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
foreach($entry in @(@('OwnerPlayer','Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'),@('Health','uint64 Unit::GetHealth(Unit* victim) const'),@('MaxHealth','uint64 Unit::GetMaxHealth(Unit* victim) const'),@('SetHealthScal','void Unit::SetHealthScal('))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",(Block $source $entry[1]))}
$first=$source.IndexOf('    damage /= victim->GetHealthMultiplierForTarget(this);',$source.IndexOf('uint32 Unit::DealDamage('))
$hook=$source.IndexOf('    if (Creature* creature = victim->ToCreature())',$first)
$body=Block $source.Substring($hook) 'if (Creature* creature = victim->ToCreature())'
[IO.File]::WriteAllText("$out/NativeScaledHook.inc",$source.Substring($first,$hook-$first)+$body)
$source=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/Multiplier.inc",(Block $source 'float Creature::GetHealthMultiplierForTarget('))
$source=Get-Content "$repo/src/common/Utilities/Util.h" -Raw
[IO.File]::WriteAllText("$out/CalculatePct.inc","template <class T, class U>`n"+(Block $source 'T CalculatePct('))
$source=Get-Content "$repo/src/server/game/DataStores/DBCEnums.h" -Raw
[IO.File]::WriteAllText("$out/CriteriaTypes.inc",(Block $source 'enum CriteriaTypes')+';')
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$argument=if($Baseline){' /DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4$argument /I`"$out`" `"$PSScriptRoot/WarswordsRetreatTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Warswords retreat failed'}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarswordsCreditCatalog.cpp -ProbeName WarswordsCreditCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native retreat catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv"
foreach($expected in @("CRITERION`t4`t29087`t92`t46982`t0","CRITERION`t5`t29088`t92`t46987`t0","SPELL`t128264`tCosmetic - Channel Omni, Blue","EFFECT`t128264`t0`t6`t4`t0`t0`t1`t0")){if($native -notcontains $expected){throw "Native retreat data changed: $expected"}}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$connection[3]
  $rows=@("SELECT 'binding',entry,ScriptName,AIName FROM creature_template WHERE ScriptName='npc_vigfus_bladewind';SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry=98007;SELECT 'spawn',id,map FROM creature WHERE id=98007;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE){throw 'Live retreat binding read failed'}
 }finally{$env:MYSQL_PWD=$previous}
 [IO.File]::WriteAllLines("$out/live.tsv",[string[]]$rows)
 if($rows.Count -ne 3 -or $rows -notcontains "binding`t98007`tnpc_vigfus_bladewind`t" -or $rows -notcontains "scaling`t98007`t98`t110" -or $rows -notcontains "spawn`t98007`t1511"){throw 'Unexpected first-boss binding/scaling/spawn context'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_warswords_retreat_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Warswords retreat failed'}}finally{$env:PYTHONPATH=$previous}
}
if($Baseline){'PASS: pinned pre-fix lethal bypass reproduced in native host; not fixed Release certification.'}else{'PASS: actual native scaled hook, target-relative health/thresholds and scoped first Vigfus retreat tested. Movement, cast, world lookup and progression sinks are controlled; full combat, group and client visuals remain separate.'}
