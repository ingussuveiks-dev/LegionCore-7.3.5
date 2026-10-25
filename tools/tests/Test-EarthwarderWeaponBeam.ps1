param([switch]$BaselineSql,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($BaselineSql -and $VerifyReleaseBinary){throw 'Baseline cannot verify repaired live selection'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/earthwarder-weapon-beam"
New-Item -ItemType Directory -Force $out | Out-Null
$repair=Get-Content "$repo/sql/updates/world/2026_10_25_04_earthwarder_weapon_beam.sql" -Raw
$migration=if($BaselineSql){''}else{$repair}
$setup=@'
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry IN(94854,96034,96468);
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
INSERT INTO creature_template SELECT * FROM saved_actor;
CREATE TEMPORARY TABLE saved_native LIKE creature_template_wdb;
INSERT INTO saved_native SELECT * FROM creature_template_wdb WHERE entry=94854;
CREATE TEMPORARY TABLE creature_template_wdb LIKE saved_native;
INSERT INTO creature_template_wdb SELECT * FROM saved_native;
CREATE TEMPORARY TABLE saved_spawn LIKE creature;
INSERT INTO saved_spawn SELECT * FROM creature WHERE id=94854;
CREATE TEMPORARY TABLE creature LIKE saved_spawn;
INSERT INTO creature SELECT * FROM saved_spawn;
CREATE TEMPORARY TABLE saved_weapon LIKE gameobject;
INSERT INTO saved_weapon SELECT * FROM gameobject WHERE id=248831;
CREATE TEMPORARY TABLE gameobject LIKE saved_weapon;
INSERT INTO gameobject SELECT * FROM saved_weapon;
'@
$reset=@'
DELETE FROM creature_template;
INSERT INTO creature_template SELECT * FROM saved_actor;
UPDATE creature_template SET flags_extra=0,AIName='',ScriptName='' WHERE entry=94854;
DELETE FROM creature_template_wdb;
INSERT INTO creature_template_wdb SELECT * FROM saved_native;
DELETE FROM creature;
INSERT INTO creature SELECT * FROM saved_spawn;
DELETE FROM gameobject;
INSERT INTO gameobject SELECT * FROM saved_weapon;
'@
$sql="$setup`n$reset`n$migration`nSELECT 'selected',flags_extra FROM creature_template WHERE entry=94854;`n$migration`nSELECT 'repeat',flags_extra FROM creature_template WHERE entry=94854;"
$sql+=@'
SELECT 'preserved',COUNT(*) FROM creature_template actor JOIN saved_actor old USING(entry)
WHERE actor.unit_flags=old.unit_flags AND actor.unit_flags2=old.unit_flags2 AND actor.npcflag=old.npcflag
AND actor.npcflag2=old.npcflag2 AND actor.scale=old.scale AND actor.AIName=old.AIName AND actor.ScriptName=old.ScriptName;
SELECT 'neighbours',COUNT(*) FROM creature_template actor JOIN saved_actor old USING(entry)
WHERE actor.entry<>94854 AND actor.flags_extra=old.flags_extra;
SELECT 'spawn',modelid,phaseId,position_x,position_y,position_z FROM creature WHERE guid=361416;
SELECT 'gate',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541
AND SourceEntry=3 AND ElseGroup=0 AND ConditionTypeOrReference=44 AND ConditionValue1=909
AND ConditionValue2=5 AND ConditionValue3=0 AND NegativeCondition=0;
'@
$variants=[ordered]@{
 flags="UPDATE creature_template SET flags_extra=4096 WHERE entry=94854;"
 script="UPDATE creature_template SET ScriptName='custom' WHERE entry=94854;"
 ai="UPDATE creature_template SET AIName='custom' WHERE entry=94854;"
 display1="UPDATE creature_template_wdb SET Displayid1=1 WHERE entry=94854;"
 display2="UPDATE creature_template_wdb SET Displayid2=1 WHERE entry=94854;"
 display3="UPDATE creature_template_wdb SET Displayid3=1 WHERE entry=94854;"
 display4="UPDATE creature_template_wdb SET Displayid4=1 WHERE entry=94854;"
 spawn_id="UPDATE creature SET id=94855 WHERE guid=361416;"
 spawn_guid="UPDATE creature SET guid=361417 WHERE guid=361416;"
 spawn_map="UPDATE creature SET map=1511 WHERE guid=361416;"
 spawn_model="UPDATE creature SET modelid=33225 WHERE guid=361416;"
 spawn_phase="UPDATE creature SET phaseId='custom' WHERE guid=361416;"
 absent_spawn="DELETE FROM creature WHERE guid=361416;"
 second_spawn="UPDATE creature SET guid=361417 WHERE guid=361416; INSERT INTO creature SELECT * FROM saved_spawn;"
 weapon_id="UPDATE gameobject SET id=248832 WHERE guid=134175;"
 weapon_guid="UPDATE gameobject SET guid=134176 WHERE guid=134175;"
 weapon_map="UPDATE gameobject SET map=1511 WHERE guid=134175;"
 weapon_phase="UPDATE gameobject SET phaseId='custom' WHERE guid=134175;"
 absent_weapon="DELETE FROM gameobject WHERE guid=134175;"
}
foreach($name in $variants.Keys){$sql+="`n$reset`n$($variants[$name])`n$repair`nSELECT '$name',flags_extra FROM creature_template WHERE entry=94854;"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previousPassword=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Temporary weapon-beam fixture failed'}
}finally{$env:MYSQL_PWD=$previousPassword}
$selected=if($BaselineSql){0}else{128}
foreach($expected in @("selected`t$selected","repeat`t$selected","preserved`t3","neighbours`t2","spawn`t0`t6363 5048 7000`t4700.21`t352.01`t-37.4104","gate`t1")){if($rows -notcontains $expected){throw "Weapon-beam preservation differs: $expected"}}
foreach($name in $variants.Keys){$expected=if($name -eq 'flags'){4096}else{0};if($rows -notcontains "$name`t$expected"){throw "Guard changed custom $name"}}
function Export-Function([string]$source,[string]$signature,[string]$name){
 $first=$source.IndexOf($signature);if($first -lt 0){throw "Missing native function $signature"}
 $last=$source.IndexOf('{',$first)+1;$depth=1
 while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last}
 [IO.File]::WriteAllText("$out/$name.inc",$source.Substring($first,$last-$first))
}
$source=Get-Content "$repo/src/server/game/Globals/ObjectMgr.cpp" -Raw
Export-Function $source 'int32 ObjectMgr::ChooseDisplayId(' 'ChooseDisplayId'
$array=[regex]::Match($source,'uint32 trigggerCreatureModelFileID\[5\] = \{[^}]+\};')
$first=$source.IndexOf('        if (CreatureModelDataEntry const* modelData = sCreatureModelDataStore.LookupEntry(creatureDisplay->ModelID))');$last=$source.IndexOf('        ++count;',$first)
if(!$array.Success -or $first -lt 0 -or $last -lt $first){throw 'Native trigger classification missing'}
[IO.File]::WriteAllText("$out/Classify.inc",$array.Value+"`n"+$source.Substring($first,$last-$first))
$source=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
foreach($name in @('GetRandomValidModelId','GetFirstValidModelId')){Export-Function $source "int32 CreatureTemplate::$name() const" $name}
foreach($name in @('GetFirstInvisibleModel','GetFirstVisibleModel')){Export-Function $source "uint32 CreatureTemplate::$name() const" $name}
$first=$source.IndexOf('    // Cancel load if no model defined',$source.IndexOf('bool Creature::InitEntry('));$last=$source.IndexOf('    // Load creature equipment',$first)
if($first -lt 0 -or $last -lt $first){throw 'Native model init missing'}
[IO.File]::WriteAllText("$out/InitEntryModel.inc",$source.Substring($first,$last-$first))
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$first=$source.IndexOf('                uint32 displayId = m_uint32Values[index];');$last=$source.IndexOf('                *data << uint32(displayId);',$first)+'                *data << uint32(displayId);'.Length
if($first -lt 0 -or $last -lt $first){throw 'Native client display missing'}
[IO.File]::WriteAllText("$out/ClientDisplay.inc",$source.Substring($first,$last-$first))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /DARTIFACT_MODEL_ENTRY=94854 /DARTIFACT_MODEL_DISPLAY=64600 /I`"$out`" `"$PSScriptRoot/TalonclawWorldModelTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" $selected`n")
$nativeOutput=@(& "$out/run.cmd" 2>&1)
$nativeOutput | Write-Output
if($BaselineSql){if(!$LASTEXITCODE -or ($nativeOutput -join "`n") -notmatch 'Assertion failed: actor.display == ARTIFACT_MODEL_DISPLAY'){throw 'Expected baseline model assertion did not reproduce'};'BASELINE: native random selection can send33225 without state-kit58143; assertion reproduced';$global:LASTEXITCODE=0;return}
if($LASTEXITCODE){throw 'Native Earthwarder model test failed'}
$previousConfig=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile MagnarHandoffCatalog.cpp -ProbeName MagnarHandoffCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native beam graph failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previousConfig}
$native=Get-Content "$out/native.tsv"
foreach($expected in @("DISPLAY`t33225`t53`t118355`t0`t255","DISPLAY`t64600`t3923`t124640`t58143`t255","ATTACHMENT`t308634`t26686`t28`t1.5`t0`t0`t1`t1`t128288","ATTACHMENT`t309078`t26693`t28`t1`t0`t0`t1`t1`t128289","ATTACHMENT`t308635`t23902`t19`t1`t0`t250007`t1`t0`t0","ATTACHMENT`t308636`t5239`t19`t1`t0`t253568`t1`t0`t0")){if($native -notcontains $expected){throw "Native beam graph differs: $expected"}}
$hotfix=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^HotfixDatabaseInfo\s*='}).Split('"')[1].Split(';')
try{
 $env:MYSQL_PWD=$hotfix[3]
 $overlays=@('SELECT (SELECT COUNT(*) FROM spell_visual_kit_effect)+(SELECT COUNT(*) FROM spell_visual_kit_model_attach)+(SELECT COUNT(*) FROM spell_visual_effect_name);'|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($hotfix[0]) --port=$($hotfix[1]) --user=$($hotfix[2]) --database=$($hotfix[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE -or $overlays.Count -ne 1 -or $overlays[0] -ne '0'){throw 'Raw parent-record visual graph has overlays; audit them before relying on this probe'}
}finally{$env:MYSQL_PWD=$previousPassword}
if($VerifyReleaseBinary){
 try{
  $env:MYSQL_PWD=$connection[3]
  $live=@("SELECT 'live',entry,flags_extra,AIName,ScriptName FROM creature_template WHERE entry=94854; SELECT 'live-spawn',guid,id,map,modelid,phaseId FROM creature WHERE id=94854;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE -or $live -notcontains "live`t94854`t128`t`t" -or $live -notcontains "live-spawn`t361416`t94854`t1495`t0`t6363 5048 7000"){throw 'Canonical live model admission does not match repair'}
 }finally{$env:MYSQL_PWD=$previousPassword}
 $previousPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_talonclaw_world_model_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical model selection bridge missing'}}finally{$env:PYTHONPATH=$previousPython}
}
'PASS:19 custom guards/idempotence, native94854 selection/init/ordinary and GM client display, exact58143 weapon/beam graph; real rendering remains a client check'
