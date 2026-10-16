param([string]$MigrationFile="$PSScriptRoot/../../sql/updates/world/2026_10_16_10_dark_whispers_opening.sql")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/dark-whispers-opening"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Extract([string]$s,[string]$start,[string]$end,[string]$name){$a=$s.IndexOf($start);$b=$s.IndexOf($end,$a);if($a -lt 0 -or $b -lt $a){throw "Missing production section $start"};[IO.File]::WriteAllText("$out/$name.inc",$s.Substring($a,$b-$a))}
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
Extract $s '        case SMART_ACTION_WP_START:' '        case SMART_ACTION_WP_PAUSE:' 'Waypoint'
Extract $s '        case SMART_ACTION_TALK:' '        case SMART_ACTION_SIMPLE_TALK:' 'Talk'
Extract $s '    if (e.link && e.link != e.event_id)' 'void SmartScript::InstallTemplate(' 'LinkTail'
$timer=$s.Substring($s.IndexOf('    if (mUseTextTimer && me)'));$timer=$timer.Substring(0,$timer.IndexOf('void SmartScript::FillScript('));$timer=$timer.Substring(0,$timer.LastIndexOf('}'))
[IO.File]::WriteAllText("$out/TextTimer.inc",$timer)
$s=Get-Content "$repo/src/server/game/Texts/CreatureTextMgr.cpp" -Raw
Extract $s 'float CreatureTextMgr::GetRangeForChatType(' 'void CreatureTextMgr::SendSound(' 'ChatRange'
$migration=Get-Content $MigrationFile -Raw
$snapshot=@'
SELECT CreatureID,GroupID,ID,Type,Duration,Sound,BroadcastTextID FROM creature_text ORDER BY CreatureID,GroupID,ID;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_text LIKE creature_text;
INSERT INTO saved_text SELECT * FROM creature_text WHERE CreatureID IN(106692,106652);
CREATE TEMPORARY TABLE creature_text LIKE saved_text;
INSERT INTO creature_text SELECT * FROM saved_text;
UPDATE creature_text SET Type=12 WHERE CreatureID=106692 AND GroupID=0 AND ID=0;
$migration
SELECT 'FIRST';
$snapshot
$migration
SELECT 'SECOND';
$snapshot
SELECT 'END';
SELECT 'TIMING',id,link,event_type,event_param1,event_param2,action_type,action_param1,action_param2 FROM smart_scripts WHERE entryorguid=106692 AND source_type=0 AND id IN(0,1,2,3,11) ORDER BY id;
SELECT 'OTHER',COUNT(*) FROM creature_text a JOIN saved_text b USING(CreatureID,GroupID,ID,Text) WHERE NOT(a.CreatureID=106692 AND a.GroupID=0 AND a.ID=0) AND a.Type<>b.Type;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Opening SQL replay failed'}}finally{$env:MYSQL_PWD=$previous}
$a=[Array]::IndexOf($rows,'FIRST');$b=[Array]::IndexOf($rows,'SECOND');$d=[Array]::IndexOf($rows,'END')
if(($rows[($a+1)..($b-1)] -join "`n") -ne ($rows[($b+1)..($d-1)] -join "`n")){throw 'Opening migration is not idempotent'}
foreach($expected in @("106692`t0`t0`t14`t0`t0`t110315","106692`t1`t0`t12`t0`t0`t110316","106692`t2`t0`t12`t0`t0`t110317","TIMING`t0`t1`t60`t0`t0`t53`t1`t106692","TIMING`t1`t0`t61`t0`t0`t1`t0`t16500","TIMING`t2`t3`t52`t0`t106692`t1`t1`t7750","TIMING`t3`t11`t61`t0`t0`t22`t1`t0","TIMING`t11`t0`t61`t0`t0`t11`t211703`t2","OTHER`t0")){if($rows -notcontains $expected){throw "Opening route mismatch: $expected"}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/DarkWhispersOpeningTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" 14 16500`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native approach speech regression failed'}
'PASS: scoped first-line type, native broadcast/voice links and complete opening-before-pickup route preserved; repeated SQL stable. Client approach bubble and audio remain pending.'
