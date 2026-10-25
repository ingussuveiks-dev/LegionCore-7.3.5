param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot certify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/silver-hand-collapse"
New-Item -ItemType Directory -Force $out|Out-Null
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp" -Raw
$collapse=Section $source 'namespace SilverHandCollapse' 'namespace SilverHandPresentation'
if($Baseline){$pinned=(& git -C $repo show '6f8615cf:src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp') -join "`n";if($LASTEXITCODE -or $pinned.Contains('scene_silver_hand_collapse')){throw 'Pinned pre-collapse binding differs'};$collapse=$collapse.Replace('return SilverHandCollapse::Complete(player, scene, trigger);','(void)player; (void)scene; (void)trigger; return false;')}
[IO.File]::WriteAllText("$out/Collapse.inc",$collapse)
[IO.File]::WriteAllText("$out/Lifecycle.inc",(Section (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::SendSpellScene(' 'void Player::HandleArenaDeserter()'))
$handlers=Get-Content "$repo/src/server/game/Handlers/SceneHandler.cpp" -Raw
[IO.File]::WriteAllText("$out/Handlers.inc",$handlers.Substring($handlers.IndexOf('void WorldSession::HandleScenePlaybackCanceled(')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/SilverHandCollapseTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Silver Hand collapse lifecycle failed'}
if($Baseline){return}
$migration=Get-Content "$repo/sql/updates/world/2026_10_25_11_silver_hand_collapse_exit.sql" -Raw
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$rows=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Collapse query failed'};return $rows}finally{$env:MYSQL_PWD=$previous}}
$sql=@'
CREATE TEMPORARY TABLE saved_scene LIKE spell_scene;
INSERT INTO saved_scene SELECT * FROM spell_scene WHERE MiscValue IN(1267,1268);
CREATE TEMPORARY TABLE saved_events LIKE spell_scene_event;
INSERT INTO saved_events SELECT * FROM spell_scene_event WHERE MiscValue IN(1267,1268);
CREATE TEMPORARY TABLE saved_position LIKE spell_target_position;
INSERT INTO saved_position SELECT * FROM spell_target_position WHERE id=211867;
CREATE TEMPORARY TABLE spell_scene LIKE saved_scene;
CREATE TEMPORARY TABLE spell_scene_event LIKE saved_events;
CREATE TEMPORARY TABLE spell_target_position LIKE saved_position;
'@
$reset='DELETE FROM spell_scene;INSERT INTO spell_scene SELECT * FROM saved_scene;UPDATE spell_scene SET ScriptName=NULL WHERE MiscValue=1268;DELETE FROM spell_scene_event;INSERT INTO spell_scene_event SELECT * FROM saved_events;DELETE FROM spell_target_position;INSERT INTO spell_target_position SELECT * FROM saved_position;'
$sql+="$reset`nSELECT 'before',current.* FROM spell_scene current ORDER BY MiscValue;`n$migration`nSELECT 'first',current.* FROM spell_scene current ORDER BY MiscValue;`n$migration`nSELECT 'second',current.* FROM spell_scene current ORDER BY MiscValue;SELECT 'position-before',current.* FROM saved_position current;SELECT 'position-after',current.* FROM spell_target_position current;SELECT 'events-before',current.* FROM saved_events current;SELECT 'events-after',current.* FROM spell_scene_event current;`n"
$variants=[ordered]@{
 'handler'="UPDATE spell_scene SET ScriptName='custom' WHERE MiscValue=1268;"
 'package'='UPDATE spell_scene SET SceneScriptPackageID=1 WHERE MiscValue=1268;'
 'flags'='UPDATE spell_scene SET PlaybackFlags=1 WHERE MiscValue=1268;'
 'duration'='UPDATE spell_scene SET CustomDuration=1 WHERE MiscValue=1268;'
 'event'="INSERT INTO spell_scene_event(MiscValue,Event,trigerSpell,MonsterCredit,comment) VALUES(1268,'complete',1,0,'custom');"
 'missing-position'='DELETE FROM spell_target_position;'
 'foreign-map'='UPDATE spell_target_position SET target_map=1220;'
 'custom-position'='UPDATE spell_target_position SET target_position_x=1;'
 'custom-orientation'='UPDATE spell_target_position SET target_orientation=1;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',current.* FROM spell_scene current ORDER BY MiscValue;`n$migration`nSELECT '$name-after',current.* FROM spell_scene current ORDER BY MiscValue;`n"}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/fixture.tsv",[string[]]$rows)
function Rows([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
function Signature([string]$label){[string]::Join("`n",[string[]]@(Rows $label))}
foreach($pair in @(@('first','second'),@('position-before','position-after'),@('events-before','events-after'))){if((Signature $pair[0]) -cne (Signature $pair[1])){throw "Idempotence/preserved content differs: $pair"}}
foreach($name in $variants.Keys){if((Signature "$name-before") -cne (Signature "$name-after")){throw "Custom collapse definition overwritten: $name"}}
$before=@(Rows 'before');$after=@(Rows 'first')
if($before.Count -ne 2 -or $after.Count -ne 2 -or $before[0] -cne $after[0]){throw 'Unrelated1267 changed'}
$expected=$before[1].Split("`t");$expected[4]='scene_silver_hand_collapse'
if(($expected -join "`t") -cne $after[1]){throw 'More than original1268 binding changed'}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SilverHandPickupCatalog.cpp -ProbeName SilverHandPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native collapse catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv" -Raw
foreach($expected in @("COLLAPSE_SPELL`t211862`tTomb of Tyr Destroyed`t1`t0`t23`t90000`t1056","COLLAPSE_SPELL`t211867`tTomb of Tyr Destroyed`t1`t0`t0`t0`t1056","EFFECT`t211867`t0`t164`t0`t0`t211862`t1`t0","EFFECT`t211867`t1`t252`t0`t0`t0`t1`t17","EFFECT`t211867`t2`t61`t0`t50974`t0`t0`t0",'SCENE_TEXT'+"`t14101`tTomb Destroyed - Main",'pathPlayer = 16894','scene:EndScene()')){if(!$native.Contains($expected)){throw "Native collapse data changed: $expected"}}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_25_11_silver_hand_collapse_exit.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_25_11_silver_hand_collapse_exit.sql';SELECT 'binding',SceneScriptPackageID,MiscValue,PlaybackFlags,CustomDuration,ScriptName FROM spell_scene WHERE MiscValue=1268;SELECT 'position',id,target_map,target_position_x,target_position_y,target_position_z,target_orientation FROM spell_target_position WHERE id=211867;SELECT 'events',COUNT(*) FROM spell_scene_event WHERE MiscValue=1268;"
 foreach($expected in @("release`t$hash`tRELEASED","binding`t1619`t1268`t27`t0`tscene_silver_hand_collapse","position`t211867`t1539`t2136.63`t2399.31`t118.81`t0","events`t0")){if($live -notcontains $expected){throw "Live collapse mismatch: $expected"}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_collapse_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed collapse bridge missing'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: actual latest-scene lifecycle and native211867 data/unchanged DB destination, guarded idempotent binding and nine custom variants. Native cast acceptance is not destination arrival, late-scene retry or client/VFX certification.'
