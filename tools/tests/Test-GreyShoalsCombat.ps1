param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/grey-shoals-combat"
New-Item -ItemType Directory -Force $out | Out-Null
function Query([string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Grey Shoals combat SQL check failed'}}finally{$env:MYSQL_PWD=$previous}
 $rows
}
$migration=Get-Content "$repo/sql/updates/world/2026_10_18_09_grey_shoals_raider_combat.sql" -Raw
$rows=@(Query @"
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid IN(115025,115026,115031,115751) AND source_type=0;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
DELETE FROM smart_scripts WHERE entryorguid=115025 AND id IN(2,3);
CREATE TEMPORARY TABLE copies LIKE saved_smart;
INSERT INTO copies SELECT * FROM saved_smart WHERE entryorguid=115025 AND id=0;
UPDATE copies SET id=2,action_param1=213738;
INSERT INTO smart_scripts SELECT * FROM copies;
UPDATE copies SET id=3,action_param1=215377;
INSERT INTO smart_scripts SELECT * FROM copies;
UPDATE copies SET entryorguid=115024;
INSERT INTO smart_scripts SELECT * FROM copies;
UPDATE copies SET entryorguid=115025,source_type=9;
INSERT INTO smart_scripts SELECT * FROM copies;
SELECT 'before',s.* FROM smart_scripts s WHERE NOT(entryorguid=115025 AND source_type=0 AND id IN(2,3)) ORDER BY entryorguid,source_type,id;
$migration
SELECT 'after',s.* FROM smart_scripts s ORDER BY entryorguid,source_type,id;
$migration
SELECT 'repeat',s.* FROM smart_scripts s ORDER BY entryorguid,source_type,id;
UPDATE copies SET source_type=0,id=2,action_param1=186480;
INSERT INTO smart_scripts SELECT * FROM copies;
UPDATE copies SET id=3,action_param1=215377,target_type=2;
INSERT INTO smart_scripts SELECT * FROM copies;
$migration
SELECT 'external',COUNT(*) FROM smart_scripts WHERE entryorguid=115025 AND source_type=0 AND ((id=2 AND action_param1=186480) OR (id=3 AND target_type=2));
SELECT 'casts',entryorguid,id,action_param1,action_param2,target_type FROM smart_scripts WHERE entryorguid IN(115025,115026,115031,115751) AND source_type=0 AND id NOT IN(2,3) ORDER BY entryorguid,id;
"@)
$before=@($rows | Where-Object {$_ -like "before`t*"} | ForEach-Object {($_ -split "`t",2)[1]})
$after=@($rows | Where-Object {$_ -like "after`t*"} | ForEach-Object {($_ -split "`t",2)[1]})
$repeat=@($rows | Where-Object {$_ -like "repeat`t*"} | ForEach-Object {($_ -split "`t",2)[1]})
if($before.Count -ne 8 -or ($before -join '|') -ne ($after -join '|') -or ($after -join '|') -ne ($repeat -join '|') -or "external`t2" -notin $rows){throw 'Combat migration changed retained rows, neighbours, source types, external edits or was not idempotent'}
$casts=@($rows | Where-Object {$_ -like "casts`t*"} | ForEach-Object {($_ -split "`t",2)[1]})
$expected=@("115025`t0`t186480`t0`t1","115025`t1`t186476`t0`t1","115026`t0`t186302`t0`t2","115031`t0`t222484`t0`t2","115751`t0`t186327`t0`t5","115751`t1`t186338`t1`t5")
if(($casts -join '|') -ne ($expected -join '|')){throw 'Retained combat spells/targets changed'}
$casts | Set-Content "$out/casts.tsv"
if($Live){
 $liveRows=@(Query 'SELECT entryorguid,id,action_param1,action_param2,target_type FROM smart_scripts WHERE entryorguid IN(115025,115026,115031,115751) AND source_type=0 ORDER BY entryorguid,id;')
 if(($liveRows -join '|') -ne ($expected -join '|')){throw 'Live combat rows differ from validated migration'}
}
# Execute actual explicit-target preparation. Object lookup, spell admission,
# target setters and packet/world services remain fixture leaves.
$native=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
$a=$native.IndexOf('void Spell::InitExplicitTargets(');$b=$native.IndexOf('void Spell::SelectExplicitTargets()',$a)
if($a -lt 0 -or $b -le $a){throw 'Missing native target preparation'}
[IO.File]::WriteAllText("$out/InitExplicitTargets.inc",$native.Substring($a,$b-$a))
$header=Get-Content "$repo/src/server/game/Spells/SpellInfo.h" -Raw
$match=[regex]::Match($header,'(?s)enum SpellCastTargetFlags\b.*?\n\};')
if(!$match.Success){throw 'Missing native target flag enum'}
[IO.File]::WriteAllText("$out/TargetFlags.inc",$match.Value)
$smart=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$targetStart=$smart.IndexOf('ObjectList* SmartScript::GetTargets(')
$a=$smart.IndexOf('case SMART_TARGET_SELF:',$targetStart);$b=$smart.IndexOf('case SMART_TARGET_HOSTILE_SECOND_AGGRO:',$a)
$c=$smart.IndexOf('case SMART_TARGET_HOSTILE_RANDOM:',$targetStart);$d=$smart.IndexOf('case SMART_TARGET_HOSTILE_RANDOM_NOT_TOP:',$c)
if($a -lt 0 -or $b -le $a -or $c -lt 0 -or $d -le $c){throw 'Missing native SmartAI combat targets'}
[IO.File]::WriteAllText("$out/SmartTargets.inc",$smart.Substring($a,$b-$a)+$smart.Substring($c,$d-$c))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /I`"$out`" `"$PSScriptRoot/GreyShoalsCombatTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd)
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Grey Shoals native target preparation regression failed'}
'PASS: scoped repeated combat migration preserves full retained rows, other entries/source types and external edits; native SmartAI target cases and spell explicit-target preparation execute. Full cast/hit, damage, LOS/range and client remain separate.'
