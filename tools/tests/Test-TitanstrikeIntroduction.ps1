$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/titanstrike-introduction"
New-Item -ItemType Directory -Force $out | Out-Null
function Extract($s,$start,$end,$name){$a=$s.IndexOf($start);$b=$s.IndexOf($end,$a);if($a -lt 0 -or $b -lt $a){throw "Missing native section $start"};[IO.File]::WriteAllText("$out/$name.inc",$s.Substring($a,$b-$a))}
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
Extract $s 'Player* GetScriptCreditPlayer(' 'Unit* GetScriptCreditCaster(' 'CreditPlayer'
Extract $s '        case SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA:' '        case SMART_ACTION_SUMMON_CONVERSATION:' 'CriteriaAction'
Extract $s '        case SMART_ACTION_CAST:' '        case SMART_ACTION_CAST_CUSTOM:' 'CastAction'
Extract $s '    if (e.link && e.link != e.event_id)' 'void SmartScript::InstallTemplate(' 'LinkTail'
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
Extract $s 'enum SmartCastFlags' '// one line in DB' 'CastFlags'
$migration=Get-Content "$repo/sql/updates/world/2026_10_23_07_titanstrike_introduction_links.sql" -Raw
$select="SELECT entryorguid,id,link,event_type,event_flags,action_type,action_param1,action_param2,target_type,target_param1 FROM smart_scripts WHERE source_type=0 AND ((entryorguid=104904 AND id IN(0,2,3,4)) OR (entryorguid=104949 AND id IN(1,2,3,4))) ORDER BY entryorguid,id;"
$sql=@"
CREATE TEMPORARY TABLE saved_sai LIKE smart_scripts;
INSERT INTO saved_sai SELECT * FROM smart_scripts WHERE entryorguid IN (104904,104949);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_sai;
INSERT INTO smart_scripts SELECT * FROM saved_sai;
-- Normalize only the two reviewed graphs, so this also runs after deployment.
UPDATE smart_scripts SET link=2,action_type=11,action_param1=210494,action_param2=0,target_type=7 WHERE entryorguid=104904 AND source_type=0 AND id=0;
UPDATE smart_scripts SET link=3,action_type=80,action_param1=104904,action_param2=2,target_type=1 WHERE entryorguid=104904 AND source_type=0 AND id=2;
UPDATE smart_scripts SET link=0 WHERE entryorguid=104904 AND source_type=0 AND id=4;
UPDATE smart_scripts SET link=2,action_type=11,action_param1=187207,action_param2=0,target_type=19,target_param1=106302 WHERE entryorguid=104949 AND source_type=0 AND id=1;
UPDATE smart_scripts SET link=3,action_type=205,action_param1=92,action_param2=49879,target_type=21,target_param1=50 WHERE entryorguid=104949 AND source_type=0 AND id=2;
UPDATE smart_scripts SET link=0 WHERE entryorguid=104949 AND source_type=0 AND id=4;
CREATE TEMPORARY TABLE baseline_sai LIKE smart_scripts;
INSERT INTO baseline_sai SELECT * FROM smart_scripts;
SELECT 'BEFORE';
$select
$migration
SELECT 'AFTER';
$select
$migration
SELECT 'REPEAT';
$select
SELECT 'END';
SELECT 'timing',SUM(event_param1),SUM(event_param2) FROM smart_scripts WHERE entryorguid=104904 AND source_type=9;
SELECT 'missing-link',COUNT(*) FROM smart_scripts a LEFT JOIN smart_scripts b ON b.entryorguid=a.entryorguid AND b.source_type=a.source_type AND b.id=a.link AND b.event_type=61 WHERE a.link<>0 AND b.id IS NULL;
SELECT 'changed-other',COUNT(*) FROM smart_scripts a JOIN saved_sai b USING(entryorguid,source_type,id) WHERE NOT (a.source_type=0 AND ((a.entryorguid=104904 AND a.id IN(0,2,4)) OR (a.entryorguid=104949 AND a.id IN(1,2,4)))) AND (a.link<>b.link OR a.action_type<>b.action_type OR a.action_param1<>b.action_param1 OR a.event_flags<>b.event_flags);
-- A different custom graph must be left alone, including its opening row.
DELETE FROM smart_scripts;
INSERT INTO smart_scripts SELECT * FROM baseline_sai;
UPDATE smart_scripts SET action_type=99 WHERE source_type=0 AND id=2;
$migration
SELECT 'custom',COUNT(*) FROM smart_scripts WHERE source_type=0 AND id=2 AND action_type=99;
SELECT 'custom-root',COUNT(*) FROM smart_scripts WHERE source_type=0 AND action_type=11 AND ((entryorguid=104904 AND id=0 AND action_param1=210494) OR (entryorguid=104949 AND id=1 AND action_param1=187207));
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Introduction temporary SQL fixture failed'}}finally{$env:MYSQL_PWD=$previous}
$a=[Array]::IndexOf($rows,'BEFORE');$b=[Array]::IndexOf($rows,'AFTER');$d=[Array]::IndexOf($rows,'REPEAT');$e=[Array]::IndexOf($rows,'END')
$before=$rows[($a+1)..($b-1)];$after=$rows[($b+1)..($d-1)];$repeat=$rows[($d+1)..($e-1)]
if(($after -join '|') -ne ($repeat -join '|')){throw 'Introduction migration not idempotent'}
foreach($expected in @("timing`t55000`t55000","missing-link`t0","changed-other`t0","custom`t2","custom-root`t2")){if($rows -notcontains $expected){throw "Missing preservation result $expected"}}
foreach($expected in @("104904`t0`t3`t62`t1`t80`t104904`t2`t1`t0","104904`t2`t0`t61`t1`t11`t210494`t0`t7`t0","104904`t4`t2`t61`t1`t81`t0`t0`t1`t0","104949`t1`t3`t40`t1`t205`t92`t49879`t21`t50","104949`t2`t0`t61`t1`t11`t187207`t0`t19`t106302","104949`t4`t2`t61`t1`t45`t1`t1`t19`t104904")){if($after -notcontains $expected){throw "Wrong introduction row $expected"}}
$export=@($before | ForEach-Object {"BEFORE`t$_"})+@($after | ForEach-Object {"AFTER`t$_"})
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]]$export)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/TitanstrikeIntroductionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/rows.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native introduction dispatcher regression failed'}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TitanstrikeIntroCatalog.cpp -ProbeName TitanstrikeIntroCatalog -OutputFile "$out/native.tsv"
if((Get-Content "$out/native.tsv") -notcontains "CRITERIA`t1068`t47168`t47167`t1`t29684`t92`t49879"){throw 'Native meeting criterion mismatch'}
'PASS: repeat migration, valid LINKs, native 55-second dialogue and unrelated later stages preserved. Actual Stage1 traversal remains a client check.'
