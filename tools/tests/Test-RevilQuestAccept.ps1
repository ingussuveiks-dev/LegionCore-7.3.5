$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/revil-quest-accept"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Extract([string]$s,[string]$start,[string]$end,[string]$name){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf($end,$a);if($b -lt $a){throw "Missing $end"};[IO.File]::WriteAllText("$out/$name.inc",$s.Substring($a,$b-$a))}
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$a=$s.IndexOf('        case SMART_ACTION_SUMMON_CREATURE:');$sai=$s.Substring($a)
Extract $sai '            ObjectList* targets = GetTargets(e, unit);' '            if (e.GetTargetType() == SMART_TARGET_RANDOM_POSITION)' 'Summon'
$a=$s.IndexOf('    if (e.link && e.link != e.event_id)');$b=$s.IndexOf('void SmartScript::InstallTemplate',$a)
$link=$s.Substring($a,$b-$a);$link=$link.Substring(0,$link.LastIndexOf('}'))
[IO.File]::WriteAllText("$out/Link.inc",$link)
Extract $s 'SmartScriptHolder SmartScript::FindLinkedEvent(' 'void SmartScript::OnUpdate(' 'FindLink'
Extract $s '        case SMART_ACTION_WP_START:' '        case SMART_ACTION_WP_PAUSE:' 'StartAction'
Extract $s '        case SMART_ACTION_ACTIVATE_GOBJECT:' '        case SMART_ACTION_RESET_GOBJECT:' 'Gate'
Extract $s '        case SMART_ACTION_SET_NPC_FLAG:' '        case SMART_ACTION_ADD_NPC_FLAG:' 'Flags'
$s=$s.Substring($s.IndexOf('void SmartScript::ProcessEvent('))
Extract $s '        case SMART_EVENT_ACCEPTED_QUEST:' '        case SMART_EVENT_TRANSPORT_ADDCREATURE:' 'Quest'
Extract $s '        case SMART_EVENT_WAYPOINT_REACHED:' '        case SMART_EVENT_SUMMON_DESPAWNED:' 'Waypoint'
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
Extract $s 'void SmartAI::IsSummonedBy(' 'void SmartAI::DamageDealt(' 'Summoned'
Extract $s 'bool SmartAI::StartPath(' 'bool SmartAI::LoadPath(' 'StartPath'
$s=Get-Content "$repo/src/server/game/Entities/Creature/TemporarySummon.cpp" -Raw
Extract $s 'void TempSummon::InitSummon()' 'bool TempSummon::InitBaseStat(' 'InitSummon'
$migration=Get-Content "$repo/sql/updates/world/2026_10_20_07_revil_quest_accept_link.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_sai LIKE smart_scripts;
INSERT INTO saved_sai SELECT * FROM smart_scripts WHERE entryorguid=100812;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_sai;
INSERT INTO smart_scripts SELECT * FROM saved_sai;
UPDATE smart_scripts SET link=6 WHERE entryorguid=100812 AND source_type=0 AND id IN(4,5);
SELECT 'BEFORE';
SELECT * FROM smart_scripts ORDER BY source_type,id;
$migration
SELECT 'FIRST';
SELECT * FROM smart_scripts ORDER BY source_type,id;
$migration
SELECT 'SECOND';
SELECT * FROM smart_scripts ORDER BY source_type,id;
SELECT 'END';
SELECT pointid,position_x,position_y,position_z FROM waypoints WHERE entry=100812 ORDER BY pointid;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Revil SQL replay failed'}}finally{$env:MYSQL_PWD=$old}
$a=[Array]::IndexOf($rows,'BEFORE');$b=[Array]::IndexOf($rows,'FIRST');$d=[Array]::IndexOf($rows,'SECOND');$end=[Array]::IndexOf($rows,'END')
$before=@($rows[($a+1)..($b-1)]);$first=@($rows[($b+1)..($d-1)]);$second=@($rows[($d+1)..($end-1)])
if(($first -join "`n") -ne ($second -join "`n")){throw 'Migration not idempotent'}
$expected=@($before | ForEach-Object {$f=$_.Split("`t");if($f[1] -eq '0' -and $f[2] -in '4','5'){$f[3]='0'};$f -join "`t"})
if(($first -join "`n") -ne ($expected -join "`n")){throw 'Migration changed more than the two stale links'}
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]]$first)
[IO.File]::WriteAllLines("$out/points.tsv",[string[]]$rows[($end+1)..($rows.Count-1)])
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/RevilQuestAcceptTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/rows.tsv`" `"$out/points.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native Revil regression failed'}
'PASS: repeated migration changes only two stale links, preserving every other script column'
