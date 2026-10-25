param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned intro baseline cannot certify installed code'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/magnar-intro"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/EarthWarder.cpp'
$source=Get-Content "$repo/$path" -Raw
$source=$source.Substring(0,$source.IndexOf('class npc_magnar_icebreaker'))
$first=$source.IndexOf('        bool magnarIntroPending');$last=$source.IndexOf('        void sGossipSelect(',$first)
[IO.File]::WriteAllText("$out/Fields.inc",$source.Substring($first,$last-$first))
[IO.File]::WriteAllText("$out/Context.inc",(Block $source 'bool IsMagnarOfferContext('))
$intro=(Block $source 'bool IsMagnarIntroPlayer(')+(Block $source 'bool IsMagnarIntroContext(')+(Block $source 'uint32 GetData(').Replace(' override','')+(Block $source 'void CancelMagnarIntro(')+(Block $source 'void UpdateMagnarIntro(')
if($source.Contains('bool IsMagnarOfferReady(')){$intro+=(Block $source 'bool IsMagnarOfferReady(')+(Block $source 'bool IsMagnarOfferPosition(')+(Block $source 'bool HasMagnarChamberRequest(')}
if($Baseline){$source=(& git -C $repo show "8e9a0d6e:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned no-scene source unavailable'};$intro='void UpdateMagnarIntro(uint32){} uint32 GetData(uint32) const { return 0; }'}
[IO.File]::WriteAllText("$out/Intro.inc",$intro)
$action=Block $source 'void DoAction('
if($Baseline){$first=$action.IndexOf('                case 2:');$last=$action.IndexOf('                    break;',$first);if($first -lt 0 -or $last -lt 0){throw 'Pinned action2 missing'};$branch=$action.Substring($first,$last-$first)+'break;'}else{$branch=Block $action 'case 2:'}
[IO.File]::WriteAllText("$out/Action.inc",'void StartIntro(){switch(2){'+$branch+'}}')
if(!$Baseline){
 $reset=Block $source 'void Reset()';$update=Block $source 'void UpdateAI('
 if(!$reset.Contains('CancelMagnarIntro();') -or $update.IndexOf('UpdateMagnarIntro(diff);') -gt $update.IndexOf('if (!UpdateVictim())')){throw 'Native idle retry/Reset cleanup missing'}
 $handlers=Get-Content "$repo/src/server/game/Handlers/SceneHandler.cpp" -Raw
 foreach($handler in 'void WorldSession::HandleScenePlaybackCanceled(','void WorldSession::HandleScenePlaybackComplete('){if(!(Block $handlers $handler).Contains('_player->SceneCompleted(packet.SceneInstanceID);')){throw 'Native complete/cancel acknowledgement changed'}}
}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/MagnarIntroTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Magnar intro regression failed'}
if($Baseline){return}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Intro metadata query failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$migration=Get-Content "$repo/sql/updates/world/2026_10_25_06_magnar_intro_scene.sql" -Raw
$rows=Query @"
CREATE TEMPORARY TABLE saved_scene LIKE spell_scene;
CREATE TEMPORARY TABLE saved_scene_event LIKE spell_scene_event;
CREATE TEMPORARY TABLE spell_scene LIKE saved_scene;
CREATE TEMPORARY TABLE spell_scene_event LIKE saved_scene_event;
$migration
$migration
SELECT 'binding',COUNT(*) FROM spell_scene WHERE MiscValue=1059 AND SceneScriptPackageID=1458 AND PlaybackFlags=16 AND CustomDuration=0 AND ScriptName='';
UPDATE spell_scene SET SceneScriptPackageID=1,ScriptName='custom';
$migration
SELECT 'custom-scene',COUNT(*) FROM spell_scene WHERE SceneScriptPackageID=1 AND ScriptName='custom';
DELETE FROM spell_scene;
INSERT INTO spell_scene VALUES(1458,999,1,1,'custom','custom');
$migration
SELECT 'custom-package',COUNT(*) FROM spell_scene WHERE MiscValue=999 AND SceneScriptPackageID=1458;
DELETE FROM spell_scene;
INSERT INTO spell_scene_event VALUES(1059,'complete',1,0,'custom');
$migration
SELECT 'custom-event',COUNT(*) FROM spell_scene;
"@
foreach($expected in @("binding`t1","custom-scene`t1","custom-package`t1","custom-event`t0")){if($rows -notcontains $expected){throw "Non-destructive scene migration differs: $expected"}}
if($VerifyReleaseBinary){
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile MagnarIntroSceneCatalog.cpp -ProbeName MagnarIntroSceneCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native intro catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $native=Get-Content "$out/native.tsv" -Raw
 foreach($expected in @("SPELL`t189721`tMagnar Intro Scene","EFFECT`t0`t6`t430`t1059`t0`t1`t0","DURATION`t90000`t0`t90000","PACKAGE`t1458`t7.0 Artifacts - Warrior - Prot - Magnar Reveal - JMC","MEMBER`t1458`t13449`t0`t2","MEMBER`t1458`t13450`t0`t3","MEMBER`t1458`t14753`t0`t4",'local idBoss = 94699','local spawnBoss = 17917','pathCam1Source = 17918','pathCam1Target = 17919','magnar:PlaySpellImpactVisual(49290)','txtBossShout1 = 97071','Animations.EmoteRoar','Animations.Attack1HPierce','ARTIFACT_WARRIOR_PROT_BOSSNAME1','scene:SetCamera(0,0,nil,nil)','scene:EndScene()')){if(!$native.Contains($expected)){throw "Native scene graph differs: $expected"}}
 foreach($expected in @("ACTOR`t94699`tMagnar Icebreaker","MODEL`t64124`t2594`t122815","VISUAL_EVENT`t49290`t6`t13`t13827`t0")){if(!$native.Contains($expected)){throw "Native scene actor/visual differs: $expected"}}
 foreach($sound in 77391,77396,77393,73289,60406){if($native -notmatch "SOUND`t$sound`t\d+"){throw "Missing native scene soundtrack $sound"}}
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_25_06_magnar_intro_scene.sql" -Algorithm SHA1).Hash
 $rows=Query "SELECT 'live',SceneScriptPackageID,MiscValue,PlaybackFlags,CustomDuration,COALESCE(ScriptName,'') FROM spell_scene WHERE MiscValue=1059; SELECT 'events',COUNT(*) FROM spell_scene_event WHERE MiscValue=1059; SELECT 'release',hash,state FROM updates WHERE name='2026_10_25_06_magnar_intro_scene.sql';"
 if($rows -notcontains "live`t1458`t1059`t16`t0`t" -or $rows -notcontains "events`t0"){throw 'Installed intro binding differs'}
 if($rows -notcontains "release`t$hash`tRELEASED"){throw 'Installed intro migration hash differs'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_magnar_intro_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed intro policy differs'}}finally{$env:PYTHONPATH=$previous}
}
