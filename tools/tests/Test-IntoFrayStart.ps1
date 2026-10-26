param([switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/into-fray-start"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 [IO.File]::WriteAllText("$out/CanCompleteQuest.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw -Encoding UTF8) 'bool Player::CanCompleteQuest(uint32'))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $definition=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/IntoFrayCompletionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Into Fray completion regression failed'}
 $migration=Get-Content "$repo/sql/updates/world/2026_10_26_03_into_the_fray_start.sql" -Raw
 $sql=(@('quest_template','quest_template_addon','quest_objectives','quest_start_scripts','quest_end_scripts')|ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+=@"
UPDATE quest_template SET StartScript=38834,CompleteScript=0 WHERE ID=38834;
UPDATE quest_template_addon SET SpecialFlags=10 WHERE ID=38834;
DELETE FROM quest_objectives WHERE QuestID=38834;
DELETE FROM quest_start_scripts WHERE id=38834;
INSERT INTO quest_start_scripts VALUES (38834,0,7,38834,0,0,0,0,0,0),(38834,0,7,38834,0,0,0,0,0,0),(38834,0,7,38834,0,0,0,0,0,0),(38834,0,7,38834,0,0,0,0,0,0),(38834,0,7,38834,0,0,0,0,0,0),(38834,0,7,38834,0,0,0,0,0,0),(38834,0,7,38834,0,0,0,0,0,0);
DELETE FROM quest_end_scripts WHERE id=38834;
$migration
$migration
SELECT 'fixed',StartScript,CompleteScript FROM quest_template WHERE ID=38834;
SELECT 'flags',SpecialFlags FROM quest_template_addon WHERE ID=38834;
SELECT 'calls',COUNT(*) FROM quest_start_scripts WHERE id=38834;
SELECT 'others',COUNT(*) FROM quest_template a JOIN saved_quest_template b USING(ID) WHERE a.ID<>38834 AND (a.StartScript<>b.StartScript OR a.CompleteScript<>b.CompleteScript);
-- A custom start script must remain untouched, including its exploration flag.
UPDATE quest_template SET StartScript=999 WHERE ID=38834;
UPDATE quest_template_addon SET SpecialFlags=10 WHERE ID=38834;
$migration
SELECT 'custom',StartScript FROM quest_template WHERE ID=38834;
SELECT 'custom-flags',SpecialFlags FROM quest_template_addon WHERE ID=38834;
"@
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$rows=$sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Into Fray TEMP SQL failed'}}finally{$env:MYSQL_PWD=$previous}
 $expected=@("fixed`t0`t0","flags`t8","calls`t0","others`t0","custom`t999","custom-flags`t10")
 if(Compare-Object $expected $rows){throw 'Into Fray migration results differ'}
 'PASS production migration twice in session TEMP shadows only; seven exact obsolete calls removed, other flags/quests and custom binding preserved; no live DB/character write'
}
