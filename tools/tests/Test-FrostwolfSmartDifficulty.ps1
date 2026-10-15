$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/frostwolf-smart-difficulty"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_15_22_frostwolf_battleguard_smart_difficulty.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=14285 AND source_type=0;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
UPDATE smart_scripts SET Difficulties='1,3',event_flags=IF(id=0,2,3);
INSERT INTO smart_scripts SELECT 14286,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment FROM saved_smart;
SELECT 'before',id,event_flags,Difficulties FROM smart_scripts WHERE entryorguid=14285 ORDER BY id;
$migration
SELECT 'after',id,event_flags,IF(Difficulties='','-',Difficulties) FROM smart_scripts WHERE entryorguid=14285 ORDER BY id;
SELECT 'preserved',COUNT(*) FROM smart_scripts t JOIN saved_smart s ON t.id=s.id AND t.source_type=s.source_type WHERE t.entryorguid=14285 AND t.action_type=s.action_type AND t.action_param1=s.action_param1 AND t.event_type=s.event_type AND t.target_type=s.target_type AND t.event_param1=s.event_param1 AND t.event_param2=s.event_param2 AND t.event_param3=s.event_param3 AND t.event_param4=s.event_param4;
$migration
SELECT 'repeat',id,event_flags,IF(Difficulties='','-',Difficulties) FROM smart_scripts WHERE entryorguid=14285 ORDER BY id;
SELECT 'neighbour',COUNT(*) FROM smart_scripts t JOIN saved_smart s ON t.id=s.id AND t.source_type=s.source_type WHERE t.entryorguid=14286 AND t.Difficulties=s.Difficulties AND t.event_flags=s.event_flags AND t.action_param1=s.action_param1;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Temporary Frostwolf fixture failed'}
} finally {$env:MYSQL_PWD=$old}
$before=@($rows | Where-Object {$_ -like "before`t*"} | ForEach-Object {($_ -split "`t",2)[1]})
$after=@($rows | Where-Object {$_ -like "after`t*"} | ForEach-Object {($_ -split "`t",2)[1]})
$repeat=@($rows | Where-Object {$_ -like "repeat`t*"} | ForEach-Object {($_ -split "`t",2)[1]})
if($before.Count -ne 4 -or ($after -join '|') -ne ($repeat -join '|') -or "preserved`t4" -notin $rows -or "neighbour`t4" -notin $rows){throw 'Migration changed unrelated data or was not idempotent'}
[IO.File]::WriteAllLines("$out/before.tsv",[string[]]$before)
[IO.File]::WriteAllLines("$out/after.tsv",[string[]]$after)
function Export-Function($source,$signature,$name) {
 $a=$source.IndexOf($signature);if($a -lt 0){throw "Missing native function $signature"}
 $b=$source.IndexOf('{',$a)+1;$depth=1
 while($depth -and $b -lt $source.Length){if($source[$b] -eq '{'){++$depth};if($source[$b] -eq '}'){--$depth};++$b}
 [IO.File]::WriteAllText("$out/$name.inc",$source.Substring($a,$b-$a))
}
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
Export-Function $source 'void SmartScript::FillScript(' 'FillScript'
$header=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
$a=$header.IndexOf('enum SmartEventFlags');$b=$header.IndexOf('};',$a)+2
[IO.File]::WriteAllText("$out/SmartEventFlags.inc",$header.Substring($a,$b-$a))
$manager=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.cpp" -Raw
Export-Function $manager 'if (e.Difficulties.empty() && e.event.event_flags & SMART_EVENT_FLAGS_DEPRECATED)' 'DeprecatedFlags'
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/FrostwolfSmartDifficultyTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/before.tsv`" `"$out/after.tsv`"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd)
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Frostwolf production-handler regression failed'}
'PASS: isolated migration is idempotent and preserves spell actions, timings and neighbours.'
