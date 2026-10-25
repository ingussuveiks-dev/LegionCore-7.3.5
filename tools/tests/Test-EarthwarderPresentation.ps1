param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot certify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/earthwarder-presentation"
New-Item -ItemType Directory -Force $out | Out-Null
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/EarthWarder.cpp" -Raw
$presentation=Section $source 'namespace EarthwarderPresentation' 'void AddSC_EarthWarder()'
if($Baseline){$pinned=(& git -C $repo show '792eee91:src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/EarthWarder.cpp') -join "`n";if($LASTEXITCODE -or $pinned.Contains('class scene_earthwarder_pickup')){throw 'Pinned pre-hook baseline differs'};$presentation=$presentation.Replace('return EarthwarderPresentation::Complete(player, scene, trigger);','(void)player; (void)scene; (void)trigger; return false;')}
[IO.File]::WriteAllText("$out/Presentation.inc",$presentation)
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
[IO.File]::WriteAllText("$out/Lifecycle.inc",(Section $source 'void Player::SendSpellScene(' 'void Player::HandleArenaDeserter()'))
$source=Get-Content "$repo/src/server/game/Handlers/SceneHandler.cpp" -Raw
[IO.File]::WriteAllText("$out/Handlers.inc",$source.Substring($source.IndexOf('void WorldSession::HandleScenePlaybackCanceled(')))
$source=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
[IO.File]::WriteAllText("$out/ForceEquip.inc",(Section $source 'void Spell::EffectForceEquipItem(' 'void Spell::EffectGiveArtifactPower('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/EarthwarderPresentationTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Earthwarder presentation failed'}
$migration=Get-Content "$repo/sql/updates/world/2026_10_25_01_earthwarder_scene_equip.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_scene LIKE spell_scene;
INSERT INTO saved_scene SELECT * FROM spell_scene WHERE MiscValue=1215;
CREATE TEMPORARY TABLE spell_scene LIKE saved_scene;
CREATE TEMPORARY TABLE saved_events LIKE spell_scene_event;
CREATE TEMPORARY TABLE spell_scene_event LIKE saved_events;
INSERT INTO spell_scene SELECT * FROM saved_scene;
UPDATE spell_scene SET ScriptName=NULL;
$migration
$migration
SELECT 'binding',COUNT(*) FROM spell_scene WHERE MiscValue=1215 AND ScriptName='scene_earthwarder_pickup';
UPDATE spell_scene SET ScriptName='custom_scene';
$migration
SELECT 'custom',COUNT(*) FROM spell_scene WHERE ScriptName='custom_scene';
UPDATE spell_scene SET ScriptName=NULL,SceneScriptPackageID=1;
$migration
SELECT 'custom-package',COUNT(*) FROM spell_scene WHERE ScriptName IS NULL;
UPDATE spell_scene SET SceneScriptPackageID=1575,PlaybackFlags=1;
$migration
SELECT 'custom-flags',COUNT(*) FROM spell_scene WHERE ScriptName IS NULL;
UPDATE spell_scene SET PlaybackFlags=21,CustomDuration=1;
$migration
SELECT 'custom-duration',COUNT(*) FROM spell_scene WHERE ScriptName IS NULL;
UPDATE spell_scene SET CustomDuration=0;
INSERT INTO spell_scene_event(MiscValue,Event,trigerSpell,MonsterCredit,comment) VALUES(1215,'complete',1,0,'custom');
$migration
SELECT 'custom-event',COUNT(*) FROM spell_scene WHERE ScriptName IS NULL;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Earthwarder scene query failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
foreach($expected in @("binding`t1","custom`t1","custom-package`t1","custom-flags`t1","custom-duration`t1","custom-event`t1")){if($rows -notcontains $expected){throw "Scene binding mismatch: $expected"}}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarswordsCreditCatalog.cpp -ProbeName WarswordsCreditCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native Earthwarder scene catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv" -Raw
foreach($expected in @('WEAPONID = 128288','WEAPONIDOFFHAND = 128289','WEAPONEFFECT = 9','CASTVISUALID = 60494','IMPACTVISUALID = 60494','PLAYSOUNDID = 64464','INITSOUNDID = 77359','WEAPONTEXT = "ARTIFACT_WARRIOR_PROT_WEAPONNAME"',"CHILD`t128289`t128288`t15","SCENE_MEMBER`t1575`t13960`t0","EFFECT`t208562`t1`t249`t0`t128289`t0`t1`t0","EFFECT`t208562`t2`t90`t0`t94851`t0`t1`t0","EFFECT`t225889`t0`t81`t0`t0`t203524`t1`t0")){if(!$native.Contains($expected)){throw "Native evidence changed: $expected"}}
if($VerifyReleaseBinary){
 foreach($kit in 77359,64464){if($native -notmatch "SCENE_SOUND_FILE`t$kit`t\d+"){throw "Missing native scene sound kit $kit"}}
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_25_01_earthwarder_scene_equip.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_25_01_earthwarder_scene_equip.sql'; SELECT 'scene',SceneScriptPackageID,MiscValue,PlaybackFlags,CustomDuration,ScriptName FROM spell_scene WHERE MiscValue=1215;"
 foreach($expected in @("release`t$hash`tRELEASED","scene`t1575`t1215`t21`t0`tscene_earthwarder_pickup")){if($live -notcontains $expected){throw "Live Earthwarder scene mismatch: $expected"}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_earthwarder_presentation_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Earthwarder presentation missing'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: scoped Earthwarder1215/1575 completion invokes native208562 for existing128289/128288; native lifecycle/force-equip and guarded TEMP binding checked. Full native208562 progression/group effects, return, ability bar/save and client scene remain separate.'
