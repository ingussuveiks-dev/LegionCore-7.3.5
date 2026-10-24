param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/xalatath-presentation"
New-Item -ItemType Directory -Force $out | Out-Null
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
$end=if($source.Contains('namespace XalatathPickup')){'namespace XalatathPickup'}else{'void AddSC_artifact_acquisition()'}
$presentation=Section $source 'namespace XalatathPresentation' $end
if($Baseline){$presentation=$presentation.Replace('return XalatathPresentation::Complete(player, scene, trigger);','return false;')}
[IO.File]::WriteAllText("$out/Presentation.inc",$presentation)
[IO.File]::WriteAllText("$out/Active.inc",(Section $source 'bool Active(Player*' 'bool Use(Player*'))
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
[IO.File]::WriteAllText("$out/Lifecycle.inc",(Section $source 'void Player::SendSpellScene(' 'void Player::HandleArenaDeserter()'))
$source=Get-Content "$repo/src/server/game/Handlers/SceneHandler.cpp" -Raw
[IO.File]::WriteAllText("$out/Handlers.inc",$source.Substring($source.IndexOf('void WorldSession::HandleScenePlaybackCanceled(')))
$source=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
[IO.File]::WriteAllText("$out/ForceEquip.inc",(Section $source 'void Spell::EffectForceEquipItem(' 'void Spell::EffectGiveArtifactPower('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/XalatathPresentationTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Xalatath presentation failed'}
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_37_xalatath_scene_equip.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_scene LIKE spell_scene;
INSERT INTO saved_scene SELECT * FROM spell_scene WHERE MiscValue=1187;
CREATE TEMPORARY TABLE spell_scene LIKE saved_scene;
CREATE TEMPORARY TABLE saved_events LIKE spell_scene_event;
CREATE TEMPORARY TABLE spell_scene_event LIKE saved_events;
INSERT INTO spell_scene SELECT * FROM saved_scene;
UPDATE spell_scene SET ScriptName=NULL;
$migration
$migration
SELECT 'binding',COUNT(*) FROM spell_scene WHERE MiscValue=1187 AND ScriptName='scene_xalatath_pickup';
UPDATE spell_scene SET ScriptName='custom_scene';
$migration
SELECT 'custom',COUNT(*) FROM spell_scene WHERE ScriptName='custom_scene';
UPDATE spell_scene SET ScriptName=NULL,SceneScriptPackageID=1;
$migration
SELECT 'custom-package',COUNT(*) FROM spell_scene WHERE ScriptName IS NULL;
UPDATE spell_scene SET SceneScriptPackageID=1553;
INSERT INTO spell_scene_event(MiscValue,Event,trigerSpell,MonsterCredit,comment) VALUES(1187,'complete',1,0,'custom');
$migration
SELECT 'custom-event',COUNT(*) FROM spell_scene WHERE ScriptName IS NULL;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'TEMP scene binding failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("binding`t1","custom`t1","custom-package`t1","custom-event`t1")){if($rows -notcontains $expected){throw "Scene binding mismatch: $expected"}}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_37_xalatath_scene_equip.sql" -Algorithm SHA1).Hash
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$live=@("SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_37_xalatath_scene_equip.sql'; SELECT 'scene',SceneScriptPackageID,MiscValue,PlaybackFlags,CustomDuration,ScriptName FROM spell_scene WHERE MiscValue=1187;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Live scene query failed'}}finally{$env:MYSQL_PWD=$previous}
 foreach($expected in @("release`t$hash`tRELEASED","scene`t1553`t1187`t21`t0`tscene_xalatath_pickup")){if($live -notcontains $expected){throw "Live scene binding mismatch: $expected"}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_xalatath_presentation_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical presentation missing'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: scoped scene37 binding is idempotent and preserves custom package/event/handler; native scene completion and existing inventory equip tested separately from client visuals'
