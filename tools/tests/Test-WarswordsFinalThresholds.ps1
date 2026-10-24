param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot certify repaired Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-final-thresholds"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Warrior/Warswords/Warswords.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "eb5c0e24:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned final threshold baseline unavailable'}}
$source=$source.Substring($source.IndexOf('class npc_vigfus_bladewind_last :'))
$first=$source.IndexOf('        EventMap events;');$last=$source.IndexOf('        void Reset()',$first)
[IO.File]::WriteAllText("$out/Fields.inc",$source.Substring($first,$last-$first))
[IO.File]::WriteAllText("$out/Reset.inc",(Block $source 'void Reset()'))
$pre=if($Baseline){Block $source 'void DamageTaken('}else{'void DamageTaken(Unit*, uint32&, DamageEffectType) override { }'}
$post=if($Baseline){''}else{Block $source 'void DamageTakenAfterScaling('}
[IO.File]::WriteAllText("$out/DamageTaken.inc",$pre)
[IO.File]::WriteAllText("$out/AfterScaling.inc",$post)
$source=Get-Content "$PSScriptRoot/WarswordsRetreatTest.cpp" -Raw
$first=$source.IndexOf('struct FirstBoss :')
if($first -lt 0){throw 'Shared controlled damage harness missing'}
[IO.File]::WriteAllText("$out/Harness.inc",$source.Substring(0,$first))
[IO.File]::WriteAllText("$out/Deal.inc",(Block $source 'uint32 Unit::Deal('))
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
foreach($entry in @(@('OwnerPlayer','Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'),@('Health','uint64 Unit::GetHealth(Unit* victim) const'),@('MaxHealth','uint64 Unit::GetMaxHealth(Unit* victim) const'),@('SetHealthScal','void Unit::SetHealthScal('))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",(Block $source $entry[1]))}
$first=$source.IndexOf('    damage /= victim->GetHealthMultiplierForTarget(this);',$source.IndexOf('uint32 Unit::DealDamage('));$hook=$source.IndexOf('    if (Creature* creature = victim->ToCreature())',$first)
[IO.File]::WriteAllText("$out/NativeScaledHook.inc",$source.Substring($first,$hook-$first)+(Block $source.Substring($hook) 'if (Creature* creature = victim->ToCreature())'))
$source=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/Multiplier.inc",(Block $source 'float Creature::GetHealthMultiplierForTarget('))
$source=Get-Content "$repo/src/common/Utilities/Util.h" -Raw
[IO.File]::WriteAllText("$out/CalculatePct.inc","template <class T, class U>`n"+(Block $source 'T CalculatePct('))
$source=Get-Content "$repo/src/server/game/DataStores/DBCEnums.h" -Raw
[IO.File]::WriteAllText("$out/CriteriaTypes.inc",(Block $source 'enum CriteriaTypes')+';')
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/WarswordsFinalThresholdsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Final threshold native regression failed'}
if($VerifyReleaseBinary){
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarswordsCreditCatalog.cpp -ProbeName WarswordsCreditCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native final threshold catalogue failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $native=Get-Content "$out/native.tsv"
 foreach($expected in @("BROADCAST`t100374`t60378`t0","BROADCAST`t106338`t60379`t0","BROADCAST`t100375`t60380`t0","AUDIO_FILE`t100374`t60378`t1370033","AUDIO_FILE`t106338`t60379`t1370034","AUDIO_FILE`t100375`t60380`t1370035","CRITERION`t5`t29088`t92`t46987`t0")){if($native -notcontains $expected){throw "Final native voice/scenario catalogue changed: $expected"}}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$rows=@("SELECT GroupID,BroadcastTextID FROM creature_text WHERE CreatureID=98602 ORDER BY GroupID,ID;SELECT entry,ScriptName FROM creature_template WHERE ScriptName='npc_vigfus_bladewind_last';"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Final threshold text/binding read failed'}}finally{$env:MYSQL_PWD=$previous}
 foreach($expected in @("0`t100374","1`t106338","2`t100375","98602`tnpc_vigfus_bladewind_last")){if($rows -notcontains $expected){throw "Final native text/binding changed: $expected"}}
 if($rows.Count -ne 4){throw 'Additional final text/bindings need review'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_warswords_final_thresholds_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical final thresholds missing'}}finally{$env:PYTHONPATH=$previous}
}
