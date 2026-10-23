param([switch]$BaselineScene)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/talonclaw-presentation"
New-Item -ItemType Directory -Force $out | Out-Null
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,$first);if($first -lt 0 -or $last -lt $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
[IO.File]::WriteAllText("$out/Presentation.inc",(Section $source 'namespace TalonclawPresentation' 'void AddSC_artifact_acquisition()'))
[IO.File]::WriteAllText("$out/Active.inc",(Section $source 'bool Active(Player*' 'bool Use(Player*'))
if($BaselineScene){$source=(& git -C $repo show 4449e4cc:src/server/game/Entities/Player/Player.cpp) -join "`n"}else{$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw}
[IO.File]::WriteAllText("$out/Lifecycle.inc",(Section $source 'void Player::SendSpellScene(' 'void Player::HandleArenaDeserter()'))
$source=Get-Content "$repo/src/server/game/Handlers/SceneHandler.cpp" -Raw
[IO.File]::WriteAllText("$out/Handlers.inc",$source.Substring($source.IndexOf('void WorldSession::HandleScenePlaybackCanceled(')))
$source=Get-Content "$repo/src/server/game/Spells/SpellScript.cpp" -Raw
[IO.File]::WriteAllText("$out/Owner.inc",(Section $source 'Unit* AuraScript::GetUnitOwner() const' 'DynamicObject* AuraScript::GetDynobjOwner() const'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/TalonclawPresentationTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native Talonclaw presentation regression failed'}
$migration=Get-Content "$repo/sql/updates/world/2026_10_23_11_talonclaw_presentation.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE spell_script_names (spell_id INT,ScriptName VARCHAR(128));
INSERT INTO spell_script_names VALUES(205589,'custom_scene'),(123,'other_spell');
$migration
$migration
SELECT 'binding',COUNT(*) FROM spell_script_names WHERE spell_id=205589 AND ScriptName='spell_talonclaw_presentation';
SELECT 'preserved',COUNT(*) FROM spell_script_names WHERE ScriptName IN('custom_scene','other_spell');
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Presentation temporary SQL fixture failed'}
}finally{$env:MYSQL_PWD=$previous}
if($rows -notcontains "binding`t1" -or $rows -notcontains "preserved`t2"){throw 'Presentation binding idempotence/preservation failed'}
'PASS: additive repeated scene binding preserves unrelated handlers'
