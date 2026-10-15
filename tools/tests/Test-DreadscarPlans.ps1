param([string]$Migration="$PSScriptRoot/../../sql/updates/world/2026_10_15_29_dreadscar_plans.sql",[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/dreadscar-plans"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/dreadscar_followup.cpp" -Raw
$a=$source.IndexOf('namespace DreadscarPlans');$b=$source.IndexOf('class player_dreadscar_plans',$a)
[IO.File]::WriteAllText("$out/Plans.inc",$source.Substring($a,$b-$a))
$source=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
$a=$source.IndexOf('void Creature::UpdateMovementFlags()');$b=$source.IndexOf('void Creature::UpdateStat()',$a)
[IO.File]::WriteAllText("$out/Movement.inc",$source.Substring($a,$b-$a))
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$source.IndexOf('void Player::setAdventureQuestID(');$b=$source.IndexOf('int32 Player::GetCommandCooldown()',$a)
[IO.File]::WriteAllText("$out/Adventure.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/DreadscarPlansTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Dreadscar production handler regression failed'}
$migrationSql=Get-Content $Migration -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_movement LIKE creature_template_movement;
INSERT INTO saved_movement SELECT * FROM creature_template_movement;
CREATE TEMPORARY TABLE creature_template_movement LIKE saved_movement;
INSERT INTO creature_template_movement SELECT * FROM saved_movement;
$migrationSql
$migrationSql
SELECT 'movement',Ground,Swim,Flight,Rooted FROM creature_template_movement WHERE CreatureId=101979;
SELECT 'other',COUNT(*) FROM creature_template_movement a JOIN saved_movement b USING(CreatureId) WHERE a.CreatureId<>101979 AND NOT(a.Ground<=>b.Ground AND a.Swim<=>b.Swim AND a.Flight<=>b.Flight AND a.Rooted<=>b.Rooted);
SELECT 'menu',t.gossip_menu_id,t.npcflag,t.npcflag2,o.OptionNpc FROM creature_template t JOIN gossip_menu_option o ON o.MenuID=t.gossip_menu_id AND o.OptionID=0 WHERE t.entry=101979;
SELECT 'spawn',id,PhaseId,modelid FROM creature WHERE guid=339088;
SELECT 'objective',ObjectID,Amount FROM quest_objectives WHERE QuestID=40824;
SELECT 'ender',COUNT(*) FROM creature_questender WHERE id=101921 AND quest=40824;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try {$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Dreadscar temporary SQL replay failed'}}finally{$env:MYSQL_PWD=$old}
foreach($expected in @("movement`t0`t0`t1`t1","other`t0","menu`t18747`t1`t32`t31","spawn`t101979`t6930`t0","objective`t97067`t1","ender`t1")){if($rows -notcontains $expected){throw "Dreadscar data mismatch: $expected"}}
if($VerifyReleaseBinary){
 $binary=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe"))
 if(!$binary.Contains('Dreadscar plans: restored personal Ritssyn for player %u')){throw 'Canonical Release lacks Dreadscar recovery'}
}
'PASS: native movement and zone-selection credit; repeat SQL; personal Ritssyn retry, deduplication and owner/quest/map boundaries. Client rendering/UI pending.'
