param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/dark-riders-stage-lifecycle"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$s=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/dark_riders_artifact.cpp" -Raw
$a=$s.IndexOf('namespace DarkRidersArtifact');$b=$s.IndexOf('void AddSC_dark_riders_artifact()',$a)
if($a -lt 0 -or $b -lt $a){throw 'Missing production instance'}
[IO.File]::WriteAllText("$out/LifecycleInstance.inc",$s.Substring($a,$b-$a))
$a=$s.IndexOf('namespace DarkRidersArrival');$b=$s.IndexOf('class instance_dark_riders_artifact',$a)
[IO.File]::WriteAllText("$out/DarkRidersArrival.inc",$s.Substring($a,$b-$a))
$s=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$a=$s.IndexOf('        // We do not allow players to use the hidden objects.');$b=$s.IndexOf("`n    }",$a)
if($a -lt 0 -or $b -lt $a){throw 'Missing native GO Use flag admission'}
[IO.File]::WriteAllText("$out/LifecycleGoAdmission.inc",$s.Substring($a,$b-$a))
function Extract([string]$s,[string]$start,[string]$end,[string]$name){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf($end,$a);if($b -lt $a){throw "Missing $end"};[IO.File]::WriteAllText("$out/$name.inc",$s.Substring($a,$b-$a))}
$s=Get-Content "$repo/src/server/game/AI/CreatureAI.cpp" -Raw
Extract $s 'void CreatureAI::MoveInLineOfSight_Safe(' 'bool CreatureAI::CanSeeEvenInPassiveMode()' 'ArrivalSafeLos'
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
Extract $s '        case SMART_ACTION_TALK:' '        case SMART_ACTION_SIMPLE_TALK:' 'EntranceTalk'
Extract $s '        case SMART_ACTION_INVOKER_CAST:' '        case SMART_ACTION_ADD_AURA:' 'EntranceInvokerCast'
Extract $s '        case SMART_ACTION_CROSS_CAST:' '        case SMART_ACTION_CALL_RANDOM_TIMED_ACTIONLIST:' 'EntranceCrossCast'
$event=$s.Substring($s.IndexOf('void SmartScript::ProcessEvent('))
Extract $event '        case SMART_EVENT_OOC_LOS:' '        case SMART_EVENT_IC_LOS:' 'EntranceLos'
Extract $event '        case SMART_EVENT_TEXT_OVER:' '        case SMART_EVENT_DATA_SET:' 'EntranceTextOver'
$timer=$s.Substring($s.IndexOf('    if (mUseTextTimer && me)'));$timer=$timer.Substring(0,$timer.IndexOf('void SmartScript::FillScript('));$timer=$timer.Substring(0,$timer.LastIndexOf('}'))
[IO.File]::WriteAllText("$out/EntranceTimer.inc",$timer)
$s=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
Extract $s '        case CONDITION_TYPE_MASK:' '        case CONDITION_RELATION_TO:' 'EntrancePlayerCondition'
$s=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
Extract $s '        case TARGET_UNIT_CASTER:' '        case TARGET_UNIT_MASTER:' 'EntranceCasterTarget'
$explicit=$s.Substring($s.IndexOf('void Spell::InitExplicitTargets('))
Extract $explicit '    if (WorldObject* target = m_targets.GetObjectTarget())' '    else' 'EntranceExplicitTarget'
$migration=Get-Content "$repo/sql/updates/world/2026_10_20_05_dark_riders_stage_lifecycle.sql" -Raw
$migration+="`n"+(Get-Content "$repo/sql/updates/world/2026_10_20_06_dark_riders_player_arrival.sql" -Raw)
$sql=@"
CREATE TEMPORARY TABLE saved_sai LIKE smart_scripts;
INSERT INTO saved_sai SELECT * FROM smart_scripts WHERE entryorguid IN(-369334,-369378,101282,101286,101257,100850,10085000,100344);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_sai;
INSERT INTO smart_scripts SELECT * FROM saved_sai;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry IN(-369334,-369378,101282,101286,101257,100850);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
$migration
SELECT 'FIRST';
SELECT * FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT * FROM conditions ORDER BY SourceEntry,SourceId,SourceGroup,ElseGroup,ConditionTypeOrReference;
$migration
SELECT 'SECOND';
SELECT * FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT * FROM conditions ORDER BY SourceEntry,SourceId,SourceGroup,ElseGroup,ConditionTypeOrReference;
SELECT 'END';
SELECT 'ENTRANCE',id,event_type,event_param2,target_type,link FROM smart_scripts WHERE entryorguid=-369378 AND source_type=0 ORDER BY id;
SELECT 'PLAYER',COUNT(*) FROM conditions WHERE SourceEntry=-369378 AND SourceGroup=1 AND ConditionTypeOrReference=32 AND ConditionTarget=0 AND ConditionValue1=16;
SELECT 'CROSS',id,action_type,action_param1,action_param2,action_param3,action_param4,target_type,link FROM smart_scripts WHERE entryorguid=101257 AND source_type=0 AND id IN(2,3) ORDER BY id;
SELECT 'LINK',link FROM smart_scripts WHERE entryorguid=101286 AND source_type=0 AND id=0;
SELECT 'INDEPENDENT',event_type,event_param1,event_param2,action_type,action_param1 FROM smart_scripts WHERE entryorguid=101286 AND source_type=0 AND id=91;
SELECT 'RANGE',target_param1 FROM smart_scripts WHERE entryorguid=100850 AND source_type=9 AND id=2;
SELECT 'SHARED',COUNT(*) FROM smart_scripts a JOIN saved_sai b USING(entryorguid,source_type,id) WHERE a.entryorguid=101282 AND a.event_type=b.event_type AND a.target_type=b.target_type AND a.event_param2=b.event_param2;
SELECT 'CREDIT',action_param2,target_param1 FROM smart_scripts WHERE action_type=205 AND action_param1=92 AND target_type=18 AND action_param2 IN(47983,47984,47985,47987,47988,47989) ORDER BY action_param2;
SELECT 'ARRIVAL',id,link,event_type,event_flags,event_param1,event_param2,action_type,action_param1,action_param2,target_type FROM smart_scripts WHERE entryorguid=-369334 AND source_type=0 AND id>=7 ORDER BY id;
SELECT 'ARRIVAL_PLAYER',COUNT(*) FROM conditions WHERE SourceEntry=-369334 AND SourceGroup=8 AND ConditionTypeOrReference=32 AND ConditionTarget=0 AND ConditionValue1=16;
SELECT 'ARRIVAL_STAGE',COUNT(*) FROM conditions WHERE SourceEntry=-369334 AND SourceGroup=8 AND ConditionTypeOrReference=44 AND ConditionTarget=1 AND ConditionValue1=988 AND ConditionValue2=0;
SELECT 'BARRIER_GUIDE',ROUND(target_x,1),ROUND(target_y,2),ROUND(target_z,4) FROM smart_scripts WHERE entryorguid=-369334 AND source_type=0 AND id=8;
SELECT 'SHARED_ARIDEN',COUNT(*) FROM smart_scripts a JOIN saved_sai b USING(entryorguid,source_type,id) WHERE a.entryorguid=100850 AND a.event_type=b.event_type AND a.action_type=b.action_type AND a.target_type=b.target_type AND a.target_param2=b.target_param2;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$savedPwd=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Stage lifecycle SQL replay failed'}}finally{$env:MYSQL_PWD=$savedPwd}
$a=[Array]::IndexOf($rows,'FIRST');$b=[Array]::IndexOf($rows,'SECOND');$d=[Array]::IndexOf($rows,'END')
if(($rows[($a+1)..($b-1)] -join "`n") -ne ($rows[($b+1)..($d-1)] -join "`n")){throw 'Lifecycle migration is not repeatable'}
foreach($expected in @("ENTRANCE`t0`t10`t2`t1`t0","ENTRANCE`t1`t52`t101282`t7`t2","ENTRANCE`t2`t61`t0`t1`t0","PLAYER`t1","CROSS`t2`t86`t199626`t2`t18`t100`t1`t0","CROSS`t3`t86`t199627`t2`t18`t100`t1`t0","LINK`t0","INDEPENDENT`t38`t1`t1`t18`t768","RANGE`t100","SHARED`t23")){if($rows -notcontains $expected){throw "Lifecycle SQL assertion missing: $expected"}}
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]]$rows)
foreach($asset in @(47983,47984,47985,47987,47988,47989)){if($rows -notcontains "CREDIT`t$asset`t100"){throw "Missing bounded stage credit radius for $asset"}}
foreach($expected in @("ARRIVAL`t7`t8`t10`t1`t0`t48`t205`t92`t47983`t7","ARRIVAL`t8`t9`t61`t1`t0`t0`t12`t101282`t8`t8","ARRIVAL`t9`t0`t61`t1`t0`t0`t85`t199627`t2`t7","ARRIVAL_PLAYER`t1","ARRIVAL_STAGE`t1","BARRIER_GUIDE`t-10962.5`t-1970.73`t-49.8162","SHARED_ARIDEN`t13")){if($rows -notcontains $expected){throw "Player arrival SQL assertion missing: $expected"}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/DarkRidersStageLifecycleTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native stage lifecycle regression failed'}
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/DarkRidersEntranceTest.cpp`" /Fe`"$out/entrance.exe`" /Fo`"$out/entrance.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/entrance.exe`"`r`n"
[IO.File]::WriteAllText("$out/entrance-run.cmd",$cmd);& "$out/entrance-run.cmd"
if($LASTEXITCODE){throw 'Native entrance/companion regression failed'}
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/DarkRidersArrivalTest.cpp`" /Fe`"$out/arrival.exe`" /Fo`"$out/arrival.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/arrival.exe`"`r`n"
[IO.File]::WriteAllText("$out/arrival-run.cmd",$cmd);& "$out/arrival-run.cmd"
if($LASTEXITCODE){throw 'Native stationary player arrival regression failed'}
'PASS: scoped entrance bootstrap, Player invoker gate, independent follower flags, player self-casts, barrier radius and repeated SQL'
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile UlthaleshDialogueCatalog.cpp -ProbeName UlthaleshDialogueCatalog -OutputFile "$out/native.tsv"
$native=Get-Content "$out/native.tsv"
foreach($expected in @('explicit 199626 0','explicit 199627 0')){if($native -notcontains $expected){throw "Companion explicit target mask mismatch: $expected"}}
foreach($expected in @('criterion 0 92 47983','criterion 1 92 47984','criterion 2 92 47985','criterion 3 92 47987','criterion 4 92 47988','criterion 5 92 47989','criterion 6 36 128942','effect 199625 0 28 0 101286 3730 0 142','effect 199626 0 6 428 0 0 199625 1','effect 199627 0 203 0 0 0 199626 1','effect 199627 1 160 0 0 0 182603 1','effect 199627 2 160 0 0 0 222599 1')){if($native -notcontains $expected){throw "Native lifecycle data mismatch: $expected"}}
if($VerifyReleaseBinary){
 $map=Get-Content "$repo/build-extractors/bin/Release/worldserver.map" -Raw
 foreach($symbol in @('?Sync@Instance@instance_dark_riders_artifact@@','?OnCreatureCreate@Instance@instance_dark_riders_artifact@@','?OnGameObjectCreate@Instance@instance_dark_riders_artifact@@','?onScenarionNextStep@Instance@instance_dark_riders_artifact@@')){if(!$map.Contains($symbol)){throw "Canonical link map missing lifecycle handler $symbol"}}
 'PASS: canonical map includes lifecycle handlers (inspect deployed code separately)'
}
