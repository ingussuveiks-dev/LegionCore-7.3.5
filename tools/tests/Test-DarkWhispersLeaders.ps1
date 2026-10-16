param([string]$MigrationFile="$PSScriptRoot/../../sql/updates/world/2026_10_16_09_dark_whispers_leaders.sql")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/dark-whispers-leaders"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Extract([string]$s,[string]$start,[string]$end,[string]$name){$a=$s.IndexOf($start);$b=$s.IndexOf($end,$a);if($a -lt 0 -or $b -lt $a){throw "Missing native section $start"};[IO.File]::WriteAllText("$out/$name.inc",$s.Substring($a,$b-$a))}
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
Extract $s '        case SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA:' '        case SMART_ACTION_SUMMON_CONVERSATION:' 'CriteriaAction'
Extract $s '        case SMART_ACTION_CAST:' '        case SMART_ACTION_CAST_CUSTOM:' 'CastAction'
Extract $s '        case SMART_TARGET_PLAYER_DISTANCE:' '        case SMART_TARGET_STORED:' 'PlayerDistance'
Extract $s '        case SMART_TARGET_ACTION_INVOKER:' '        case SMART_TARGET_ACTION_INVOKER_VEHICLE:' 'InvokerTarget'
$s=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
Extract $s '        case CONDITION_NEAR_CREATURE:' '        case CONDITION_NEAR_GAMEOBJECT:' 'NearCondition'
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
Extract $s 'enum SmartCastFlags' '// one line in DB' 'CastFlags'
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/DarkWhispersLeadersTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Dark Whispers leaders native dispatcher regression failed'}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile DarkWhispersCatalog.cpp -ProbeName DarkWhispersCatalog -OutputFile "$out/native.tsv"
$native=Get-Content "$out/native.tsv"
foreach($row in @('step 2477 3 48047','step 2478 4 48050','tree 48048 48047 1 0 30105 92 50959','tree 48049 48047 3 0 30106 92 50960')){if($native -notcontains $row){throw "Native stage4 data mismatch: $row"}}
$migration=Get-Content $MigrationFile -Raw
$snapshot=@'
SELECT 'ROW',entryorguid,id,link,action_type,action_param1,action_param2,target_type,target_param1 FROM smart_scripts WHERE source_type=0 AND ((entryorguid=106825 AND id IN(3,4,5)) OR (entryorguid=106827 AND id IN(2,3,4))) ORDER BY entryorguid,id;
SELECT 'COND',SourceEntry,SourceGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry IN(106825,106827) ORDER BY SourceEntry,SourceGroup;
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
$migration
SELECT 'FIRST';
$snapshot
$migration
SELECT 'SECOND';
$snapshot
SELECT 'END';
SELECT 'WAVES',COUNT(*) FROM smart_scripts a JOIN saved_sai b USING(entryorguid,source_type,id) WHERE a.entryorguid=99739 AND (a.action_type<>b.action_type OR a.action_param1<>b.action_param1 OR a.action_param2<>b.action_param2);
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Leader SQL fixture failed'}}finally{$env:MYSQL_PWD=$previous}
$a=[Array]::IndexOf($rows,'FIRST');$b=[Array]::IndexOf($rows,'SECOND');$d=[Array]::IndexOf($rows,'END')
$first=@($rows[($a+1)..($b-1)]);$second=@($rows[($b+1)..($d-1)])
if(($first -join "`n") -ne ($second -join "`n")){throw 'Leader migration is not idempotent'}
foreach($expected in @("ROW`t106825`t3`t4`t205`t92`t50959`t18`t100","ROW`t106827`t2`t3`t205`t92`t50959`t18`t100","ROW`t106825`t5`t0`t11`t212550`t18`t18`t100","ROW`t106827`t4`t0`t11`t212550`t18`t18`t100","COND`t106825`t4`t29`t1`t106827`t50`t1","COND`t106827`t3`t29`t1`t106825`t50`t1")){if($first -notcontains $expected){throw "Missing leader lifecycle row $expected"}}
if($rows -notcontains "WAVES`t0"){throw 'Wave progression changed'}
'PASS: native stage4 requires50959 and3x50960; both last-leader death chains and base-actor conditions survive repeat SQL; wave actions preserved. Full client stage4->5 remains pending.'
