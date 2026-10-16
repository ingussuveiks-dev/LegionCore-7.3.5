param([string]$MigrationFile="$PSScriptRoot/../../sql/updates/world/2026_10_16_11_dark_whispers_binding.sql")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/dark-whispers-binding"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Section([string]$s,[string]$start,[string]$end){$a=$s.IndexOf($start);$b=$s.IndexOf($end,$a);if($a -lt 0 -or $b -lt $a){throw "Missing native section $start"};$s.Substring($a,$b-$a)}
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/RemoveAura.inc",(Section $s '        case SMART_ACTION_REMOVEAURASFROMSPELL:' '        case SMART_ACTION_FOLLOW:'))
[IO.File]::WriteAllText("$out/LinkTail.inc",(Section $s '    if (e.link && e.link != e.event_id)' 'void SmartScript::InstallTemplate('))
$summon=Section $s '        case SMART_ACTION_SUMMON_CREATURE:' '        case SMART_ACTION_SUMMON_GO:'
$a=$summon.IndexOf('            if (e.GetTargetType() == SMART_TARGET_POSITION)');$b=$summon.LastIndexOf('            break;')
[IO.File]::WriteAllText("$out/PositionSummon.inc",$summon.Substring($a,$b-$a))
$s=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/NearCondition.inc",(Section $s '        case CONDITION_NEAR_CREATURE:' '        case CONDITION_NEAR_GAMEOBJECT:'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/DarkWhispersBindingTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Dark Whispers binding dispatcher regression failed'}
$migration=Get-Content $MigrationFile -Raw
$snapshot=@'
SELECT 'DEATH',id,link,event_type,event_flags,action_type,action_param1,action_param2,target_type,target_x,target_y,target_z FROM smart_scripts WHERE entryorguid=106698 AND source_type=0 AND id=3;
SELECT 'REMOVE',id,link,event_type,action_type,action_param1,target_type,target_param1,target_param2 FROM smart_scripts WHERE entryorguid=106698 AND source_type=0 AND id=5;
SELECT 'CONDITION',SourceGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=106698;
SELECT 'FALLBACK',id,event_type,action_type,action_param1 FROM smart_scripts WHERE entryorguid=106692 AND source_type=0 AND id=12;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_sai LIKE smart_scripts;
INSERT INTO saved_sai SELECT * FROM smart_scripts;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_sai;
INSERT INTO smart_scripts SELECT * FROM saved_sai;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
UPDATE smart_scripts SET link=0 WHERE entryorguid=106698 AND source_type=0 AND id=3;
DELETE FROM smart_scripts WHERE entryorguid=106698 AND source_type=0 AND id=5;
$migration
SELECT 'FIRST';
$snapshot
$migration
SELECT 'SECOND';
$snapshot
SELECT 'END';
SELECT 'OTHER',COUNT(*) FROM smart_scripts a JOIN saved_sai b USING(entryorguid,source_type,id) WHERE a.entryorguid IN(106692,106652) AND (a.link<>b.link OR a.action_type<>b.action_type OR a.action_param1<>b.action_param1 OR a.action_param2<>b.action_param2);
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Binding SQL fixture failed'}}finally{$env:MYSQL_PWD=$previous}
$a=[Array]::IndexOf($rows,'FIRST');$b=[Array]::IndexOf($rows,'SECOND');$d=[Array]::IndexOf($rows,'END')
if(($rows[($a+1)..($b-1)] -join "`n") -ne ($rows[($b+1)..($d-1)] -join "`n")){throw 'Binding migration is not idempotent'}
foreach($expected in @("DEATH`t3`t5`t6`t1`t12`t106692`t8`t8`t1021.39`t5137.38`t58.2669","REMOVE`t5`t0`t61`t28`t211683`t19`t106693`t50","CONDITION`t4`t29`t1`t106698`t50`t1","FALLBACK`t12`t27`t28`t211683","OTHER`t0")){if($rows -notcontains $expected){throw "Binding lifecycle mismatch: $expected"}}
'PASS: last-death aura removal and original Mephistroth spawn preserved, actor-origin condition and boarding fallback; migration repairs old state and repeats safely. Client visual timing pending.'
