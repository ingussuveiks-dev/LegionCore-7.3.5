param([switch]$BaselineSql,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/talonclaw-world-model"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=if($BaselineSql){''}else{Get-Content "$repo/sql/updates/world/2026_10_23_14_talonclaw_world_model.sql" -Raw}
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry IN(100310,100005);
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
INSERT INTO creature_template SELECT * FROM saved_actor;
CREATE TEMPORARY TABLE saved_native LIKE creature_template_wdb;
INSERT INTO saved_native SELECT * FROM creature_template_wdb WHERE Entry=100310;
CREATE TEMPORARY TABLE creature_template_wdb LIKE saved_native;
INSERT INTO creature_template_wdb SELECT * FROM saved_native;
UPDATE creature_template SET flags_extra=0,AIName='',ScriptName='' WHERE entry=100310;
$migration
SELECT 'selected',flags_extra FROM creature_template WHERE entry=100310;
$migration
SELECT 'repeat',flags_extra FROM creature_template WHERE entry=100310;
SELECT 'preserved',COUNT(*) FROM creature_template actor JOIN saved_actor old USING(entry) WHERE actor.unit_flags=old.unit_flags AND actor.unit_flags2=old.unit_flags2 AND actor.npcflag=old.npcflag AND actor.npcflag2=old.npcflag2 AND actor.scale=old.scale AND actor.AIName=old.AIName AND actor.ScriptName=old.ScriptName;
SELECT 'neighbour',COUNT(*) FROM creature_template actor JOIN saved_actor old USING(entry) WHERE actor.entry=100005 AND actor.flags_extra=old.flags_extra;
UPDATE creature_template SET flags_extra=4096 WHERE entry=100310;
$migration
SELECT 'custom_flags',flags_extra FROM creature_template WHERE entry=100310;
UPDATE creature_template SET flags_extra=0,ScriptName='custom_actor' WHERE entry=100310;
$migration
SELECT 'custom_script',flags_extra FROM creature_template WHERE entry=100310;
UPDATE creature_template SET ScriptName='',AIName='custom_ai' WHERE entry=100310;
$migration
SELECT 'custom_ai',flags_extra FROM creature_template WHERE entry=100310;
UPDATE creature_template SET AIName='' WHERE entry=100310;
UPDATE creature_template_wdb SET Displayid2=123 WHERE Entry=100310;
$migration
SELECT 'custom_display',flags_extra FROM creature_template WHERE entry=100310;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'World-model temporary fixture failed'}
}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("preserved`t2","neighbour`t1","custom_flags`t4096","custom_script`t0","custom_ai`t0","custom_display`t0")){if($rows -notcontains $expected){throw "World-model preservation failure: $expected"}}
$selected=($rows | Where-Object {$_ -like "selected`t*"}).Split("`t")[1]
if($rows -notcontains "repeat`t$selected"){throw 'Repeated migration changed model admission'}
function Export-Function([string]$source,[string]$signature,[string]$name){
 $first=$source.IndexOf($signature);if($first -lt 0){throw "Missing native function $signature"}
 $last=$source.IndexOf('{',$first)+1;$depth=1
 while($depth -and $last -lt $source.Length){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last}
 [IO.File]::WriteAllText("$out/$name.inc",$source.Substring($first,$last-$first))
}
$source=Get-Content "$repo/src/server/game/Globals/ObjectMgr.cpp" -Raw
Export-Function $source 'int32 ObjectMgr::ChooseDisplayId(' 'ChooseDisplayId'
$array=[regex]::Match($source,'uint32 trigggerCreatureModelFileID\[5\] = \{[^}]+\};')
$first=$source.IndexOf('        if (CreatureModelDataEntry const* modelData = sCreatureModelDataStore.LookupEntry(creatureDisplay->ModelID))');$last=$source.IndexOf('        ++count;',$first)
if(!$array.Success -or $first -lt 0 -or $last -lt $first){throw 'Native trigger model classification missing'}
[IO.File]::WriteAllText("$out/Classify.inc",$array.Value+"`n"+$source.Substring($first,$last-$first))
$source=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
foreach($name in @('GetRandomValidModelId','GetFirstValidModelId')){Export-Function $source "int32 CreatureTemplate::$name() const" $name}
foreach($name in @('GetFirstInvisibleModel','GetFirstVisibleModel')){Export-Function $source "uint32 CreatureTemplate::$name() const" $name}
$first=$source.IndexOf('    // Cancel load if no model defined',$source.IndexOf('bool Creature::InitEntry('));$last=$source.IndexOf('    // Load creature equipment',$first)
if($last -lt $first){throw 'Native InitEntry model section missing'}
[IO.File]::WriteAllText("$out/InitEntryModel.inc",$source.Substring($first,$last-$first))
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$first=$source.IndexOf('                uint32 displayId = m_uint32Values[index];');$last=$source.IndexOf('                *data << uint32(displayId);',$first)+'                *data << uint32(displayId);'.Length
if($first -lt 0 -or $last -lt $first){throw 'Native client display field missing'}
[IO.File]::WriteAllText("$out/ClientDisplay.inc",$source.Substring($first,$last-$first))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/TalonclawWorldModelTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" $selected`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native Talonclaw world-model regression failed'}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TalonclawWorldModelCatalog.cpp -ProbeName TalonclawWorldModelCatalog -OutputFile "$out/native.tsv"
$native=Get-Content "$out/native.tsv"
foreach($expected in @("MODEL`t33225`t53`t118355`t2.25`t1`t255`t0`t0","MODEL`t66609`t3923`t124640`t1`t1`t255`t61290`t0","KIT_EFFECT`t61290`t2`t313388","ATTACHMENT`t313388`t26873`t28`t1`t0`t0`t1`t1`t128808","ATTACHMENT`t313390`t8451`t19`t1`t0`t253570`t1`t0`t0")){if($native -notcontains $expected){throw "Native world-model data mismatch: $expected"}}
if($VerifyReleaseBinary){
 $previousPython=$env:PYTHONPATH
 try {
  $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython"
  & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_talonclaw_world_model_release.py" "$repo/build-extractors/bin/Release"
  if($LASTEXITCODE){throw 'Deployed world-model selection verification failed'}
 }finally{$env:PYTHONPATH=$previousPython}
}
'PASS: idempotent guarded trigger-model migration and native item128808/state-kit61290 attachments; no client rendering claim'
