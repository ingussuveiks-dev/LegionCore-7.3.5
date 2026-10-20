param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/dark-riders-login"
New-Item -ItemType Directory -Force $out | Out-Null
$s=Get-Content "$repo/src/server/game/Entities/Player/PlayerScenario.cpp" -Raw
$a=$s.IndexOf('bool Player::ExitCompletedDarkRidersLogin()')
if($a -lt 0){throw 'Missing completed login exit'}
[IO.File]::WriteAllText("$out/CompletedExit.inc",$s.Substring($a))
$s=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$s.IndexOf('int32 Player::GetQuestObjectiveData(uint32');$b=$s.IndexOf('int32 Player::GetQuestObjectiveData(Quest const*',$a)
[IO.File]::WriteAllText("$out/ObjectiveLookup.inc",$s.Substring($a,$b-$a))
$s=Get-Content "$repo/src/server/game/Handlers/CharacterHandler.cpp" -Raw
$a=$s.IndexOf('        bool addedToLoginMap =');$b=$s.IndexOf('        sObjectAccessor->AddObject(player);',$a)
if($a -lt 0 -or $b -lt $a){throw 'Missing actual login admission'}
$admission=$s.Substring($a,$b-$a)
if(!$admission.Contains('!validLoginMap && !player->ExitCompletedDarkRidersLogin()')){throw 'Completed exit must be wired only to rejected login maps'}
[IO.File]::WriteAllText("$out/PostAddAdmission.inc",$admission)
[IO.File]::WriteAllText("$out/OldPostAddAdmission.inc",$admission.Replace('!validLoginMap && !player->ExitCompletedDarkRidersLogin()','!validLoginMap'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" /I`"$repo/src/server/game/Scenarios`" `"$PSScriptRoot/DarkRidersLoginTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Completed Dark Riders login regression failed'}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$c[3]
 $rows=@("SELECT c.id,c.map,c.PhaseId,c.position_x,c.position_y,c.position_z,q.quest,t.npcflag FROM creature c JOIN creature_questender q ON q.id=c.id AND q.quest=40623 JOIN creature_template t ON t.entry=c.id WHERE c.guid=373228;" | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Native outdoor quest ender query failed'}
}finally{$env:MYSQL_PWD=$old}
if($rows -notcontains "100812`t0`t`t-11024.6`t-2006.07`t25.1777`t40623`t2"){throw 'Outdoor return spawn/questgiver flags mismatch'}
'PASS: actual DB373228 is unphased outdoor Revil, native quest ender40623 with questgiver flag'
if($VerifyReleaseBinary){
 $map=Get-Content "$repo/build-extractors/bin/Release/worldserver.map" -Raw
 if(!$map.Contains('?ExitCompletedDarkRidersLogin@Player@@')){throw 'Canonical Release lacks completed Dark Riders login exit'}
 'PASS: canonical link map contains completed login exit; caller/instructions checked separately'
}
