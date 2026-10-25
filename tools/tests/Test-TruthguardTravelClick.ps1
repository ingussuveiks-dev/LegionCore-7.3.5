param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned baseline cannot verify current Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/truthguard-travel-click"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$migration=Get-Content "$repo/sql/updates/world/2026_10_25_08_truthguard_travel_click.sql" -Raw
$reset=@'
DELETE FROM creature_template;
INSERT INTO creature_template SELECT * FROM fixture_template;
DELETE FROM npc_spellclick_spells;
INSERT INTO npc_spellclick_spells VALUES(105883,209884,1,0,1),(106203,210358,1,0,1),(105886,52391,1,0,0);
DELETE FROM conditions;
DELETE FROM smart_scripts;
'@
$digest="SELECT GROUP_CONCAT(CONCAT_WS(',',npc_entry,spell_id,cast_flags,user_type,add_npc_flag) ORDER BY npc_entry,spell_id SEPARATOR ';') INTO @click_before FROM npc_spellclick_spells;"
$sql=@"
CREATE TEMPORARY TABLE fixture_template LIKE creature_template;
INSERT INTO fixture_template SELECT * FROM creature_template WHERE entry IN(105883,106203,105886,106205);
UPDATE fixture_template SET npcflag=16777216,VehicleId=0,AIName='',ScriptName='npc_sr_argent_hippogryph' WHERE entry IN(105883,106203);
CREATE TEMPORARY TABLE creature_template LIKE fixture_template;
CREATE TEMPORARY TABLE fixture_click LIKE npc_spellclick_spells;
CREATE TEMPORARY TABLE npc_spellclick_spells LIKE fixture_click;
CREATE TEMPORARY TABLE fixture_condition LIKE conditions;
CREATE TEMPORARY TABLE conditions LIKE fixture_condition;
CREATE TEMPORARY TABLE fixture_smart LIKE smart_scripts;
CREATE TEMPORARY TABLE smart_scripts LIKE fixture_smart;
$reset
SELECT 'baseline',npc_entry,spell_id,cast_flags,user_type,add_npc_flag FROM npc_spellclick_spells WHERE npc_entry IN(105883,106203) ORDER BY npc_entry;
$migration
SELECT 'fixed',npc_entry,spell_id,cast_flags,user_type,add_npc_flag FROM npc_spellclick_spells WHERE npc_entry IN(105883,106203) ORDER BY npc_entry;
SELECT 'remaining',COUNT(*) FROM npc_spellclick_spells;
$digest
$migration
SELECT 'idempotent',@click_before=GROUP_CONCAT(CONCAT_WS(',',npc_entry,spell_id,cast_flags,user_type,add_npc_flag) ORDER BY npc_entry,spell_id SEPARATOR ';') FROM npc_spellclick_spells;
SELECT 'templates',COUNT(*) FROM creature_template current JOIN fixture_template old USING(entry) WHERE current.npcflag=old.npcflag AND current.VehicleId=old.VehicleId AND current.AIName=old.AIName AND current.ScriptName=old.ScriptName;
"@
$variants=[ordered]@{
 'custom-script'="UPDATE creature_template SET ScriptName='custom' WHERE entry=105883;"
 'custom-ai'="UPDATE creature_template SET AIName='SmartAI' WHERE entry=106203;"
 'custom-flags'='UPDATE creature_template SET npcflag=npcflag|1 WHERE entry=105883;'
 'custom-vehicle'='UPDATE creature_template SET VehicleId=4726 WHERE entry=106203;'
 'missing-row'='DELETE FROM npc_spellclick_spells WHERE npc_entry=105883;'
 'custom-cast'='UPDATE npc_spellclick_spells SET cast_flags=3 WHERE npc_entry=106203;'
 'custom-spell'='UPDATE npc_spellclick_spells SET spell_id=52391 WHERE npc_entry=105883;'
 'custom-user'='UPDATE npc_spellclick_spells SET user_type=1 WHERE npc_entry=106203;'
 'custom-add-flag'='UPDATE npc_spellclick_spells SET add_npc_flag=0 WHERE npc_entry=105883;'
 'extra-row'='INSERT INTO npc_spellclick_spells VALUES(105883,52391,1,0,1);'
 'custom-condition'='INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(18,105883,209884,9,42017);'
 'custom-ai-event'="INSERT INTO smart_scripts(entryorguid,source_type,id,comment) VALUES(-382186,0,0,'fixture');"
}
foreach($variant in $variants.GetEnumerator()){
 $sql+="`n$reset`n$($variant.Value)`n$digest`n$migration`nSELECT 'preserved-$($variant.Key)',@click_before=GROUP_CONCAT(CONCAT_WS(',',npc_entry,spell_id,cast_flags,user_type,add_npc_flag) ORDER BY npc_entry,spell_id SEPARATOR ';') FROM npc_spellclick_spells;`n"
}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$statement){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($statement|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Travel TEMP/read-only query failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/data.tsv",[string[]]$rows)
foreach($expected in @("remaining`t1","idempotent`t1","templates`t4")+@($variants.Keys|ForEach-Object {"preserved-$_`t1"})){if($rows -notcontains $expected){throw "Missing travel SQL assertion: $expected"}}
if(@($rows|Where-Object {$_ -like "fixed`t*"}).Count){throw 'Duplicate pre-AI rows remain after migration'}
$prefix=if($Baseline){'baseline'}else{'fixed'}
[IO.File]::WriteAllLines("$out/click.tsv",[string[]]@($rows|Where-Object {$_ -like "$prefix`t*"}|ForEach-Object {$_.Substring($prefix.Length+1)}))
$path='src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/shields_rest.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "edca8e77:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned travel source missing'}}
$actor=Block $source 'class npc_sr_argent_hippogryph'
[IO.File]::WriteAllText("$out/Click.inc",(Block $actor 'void OnSpellClick('))
[IO.File]::WriteAllText("$out/Summon.inc",$(if($Baseline){''}else{Block $actor 'void SummonForPlayer('}))
[IO.File]::WriteAllText("$out/NativeCastResults.inc",(Block (Get-Content "$repo/src/server/game/Miscellaneous/SharedDefines.h" -Raw) 'enum SpellCastResult')+';')
[IO.File]::WriteAllText("$out/NativeData.inc",(Block (Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/shields_rest.h" -Raw) 'enum Data')+';')
[IO.File]::WriteAllText("$out/Dispatch.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw) 'bool Unit::HandleSpellClick('))
[IO.File]::WriteAllText("$out/Visibility.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'bool Player::canSeeSpellClickOn('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$out`" `"$PSScriptRoot/TruthguardTravelClickTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/click.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native travel click host failed'}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_25_08_truthguard_travel_click.sql" -Algorithm SHA1).Hash
 $live=Query "SET SESSION TRANSACTION READ ONLY; SELECT 'release',hash,state FROM updates WHERE name='2026_10_25_08_truthguard_travel_click.sql'; SELECT 'clicks',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry IN(105883,106203); SELECT 'visible',COUNT(*) FROM creature_template WHERE entry IN(105883,106203) AND npcflag=16777216 AND VehicleId=0 AND AIName='' AND ScriptName='npc_sr_argent_hippogryph'; SELECT 'path',id,COUNT(*),MIN(point),MAX(point) FROM waypoint_data_script WHERE id IN(10588600,10620500) GROUP BY id;"
 [IO.File]::WriteAllLines("$out/live.tsv",[string[]]$live)
 foreach($expected in @("release`t$hash`tRELEASED","clicks`t0","visible`t2","path`t10588600`t5`t1`t5","path`t10620500`t5`t1`t5")){if($live -notcontains $expected){throw "Travel deployment mismatch: $expected"}}
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TruthguardPickupCatalog.cpp -ProbeName TruthguardPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if($LASTEXITCODE){throw 'Travel DB2 probe failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $native=Get-Content "$out/native.tsv"
 foreach($expected in @("EFFECT`t209884`t0`t28`t0`t105886`t0`t46`t0","EFFECT`t209884`t1`t167`t0`t0`t0`t1`t0","EFFECT`t210358`t0`t28`t0`t106205`t0`t46`t0","EFFECT`t210358`t1`t6`t4`t0`t0`t1`t0","EFFECT`t52391`t0`t6`t236`t0`t0`t25`t0")){if($native -notcontains $expected){throw "Travel native metadata changed: $expected"}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_truthguard_travel_click_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed travel click guard failed'}}finally{$env:PYTHONPATH=$previous}
}
"PASS: travel migration twice applied in TEMP tables, $($variants.Count) custom-scope variants preserved, unrelated click and all four templates unchanged; controlled native dispatch separated from live boarding/client checks"
