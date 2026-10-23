param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-return-admission"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_23_21_marksmanship_return_admission.sql" -Raw
$rollback=$migration.Replace('COMMIT;','ROLLBACK;')
$reset=@"
DELETE FROM conditions WHERE (SourceTypeOrReferenceId=15 AND SourceGroup=19331 AND SourceEntry=0 AND SourceId=0) OR (SourceTypeOrReferenceId=22 AND SourceGroup=60 AND SourceEntry=100397 AND SourceId=0);
UPDATE smart_scripts SET id=55 WHERE entryorguid=100397 AND source_type=0 AND id=59 AND event_type=62 AND action_type=33 AND action_param1=103481;
"@
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=100397;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
INSERT INTO creature_template SELECT * FROM saved_actor;
CREATE TEMPORARY TABLE saved_menu LIKE gossip_menu_option;
INSERT INTO saved_menu SELECT * FROM gossip_menu_option WHERE MenuID=19331;
CREATE TEMPORARY TABLE gossip_menu_option LIKE saved_menu;
INSERT INTO gossip_menu_option SELECT * FROM saved_menu;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=100397 AND source_type=0;
UPDATE saved_smart SET id=55 WHERE id=59 AND event_type=62 AND action_type=33 AND action_param1=103481;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceEntry=100397 OR (SourceTypeOrReferenceId=15 AND SourceGroup=19331);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
$reset
$migration
$migration
SELECT 'menu-count',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=19331 AND SourceEntry=0 AND SourceId=0;
SELECT 'event-count',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=60 AND SourceEntry=100397 AND SourceId=0;
SELECT 'same-conditions',COUNT(*) FROM conditions menu JOIN conditions choice ON menu.ElseGroup=choice.ElseGroup AND menu.ConditionTypeOrReference=choice.ConditionTypeOrReference AND menu.ConditionTarget=choice.ConditionTarget AND menu.ConditionValue1=choice.ConditionValue1 AND menu.ConditionValue2=choice.ConditionValue2 AND menu.ConditionValue3=choice.ConditionValue3 AND menu.NegativeCondition=choice.NegativeCondition WHERE menu.SourceTypeOrReferenceId=15 AND menu.SourceGroup=19331 AND menu.SourceEntry=0 AND choice.SourceTypeOrReferenceId=22 AND choice.SourceGroup=60 AND choice.SourceEntry=100397;
SELECT 'row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=60 AND SourceEntry=100397 AND SourceId=0 ORDER BY ElseGroup,ConditionTypeOrReference;
SELECT 'actor-row',id,link,event_type,action_type,action_param1,action_param2,target_type,event_flags,event_phase_mask FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id IN(54,55,56,59) ORDER BY id;
SELECT 'flag-unfiltered',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=56 AND SourceEntry=100397 AND SourceId=0;
SELECT 'unchanged',COUNT(*) FROM smart_scripts current JOIN saved_smart old ON current.entryorguid=old.entryorguid AND current.source_type=old.source_type AND current.id=old.id AND current.event_type=old.event_type AND current.link=old.link WHERE NOT(old.id=55 AND old.event_type=62) AND current.action_type=old.action_type AND current.action_param1=old.action_param1 AND current.action_param2=old.action_param2 AND current.target_type=old.target_type AND current.event_flags=old.event_flags;
SELECT 'saved-unchanged',COUNT(*) FROM saved_smart WHERE NOT(id=55 AND event_type=62);
UPDATE smart_scripts SET event_flags=129 WHERE entryorguid=100397 AND source_type=0 AND id=59 AND event_type=62;
$migration
SELECT 'other-flags',COUNT(*) FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=59 AND event_type=62 AND event_flags=128;
SELECT 'return-destination',COUNT(*) FROM spell_target_position WHERE id=205040 AND target_map=1220 AND ABS(target_position_x+837)<0.01 AND ABS(target_position_y-4330.1)<0.01 AND ABS(target_position_z-745)<0.01;
$reset
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES(15,19331,0,0,0,4,0,1,0,0,0,0,'','Custom menu'),(22,60,100397,0,0,4,0,1,0,0,0,0,'','Custom choice');
$migration
SELECT 'custom-lists',COUNT(*) FROM conditions WHERE ConditionTypeOrReference=4 AND ConditionValue1=1 AND ((SourceTypeOrReferenceId=15 AND SourceGroup=19331) OR (SourceTypeOrReferenceId=22 AND SourceGroup=60 AND SourceEntry=100397));
$reset
UPDATE creature_template SET ScriptName='custom_actor' WHERE entry=100397;
$migration
SELECT 'custom-script',COUNT(*) FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=55 AND event_type=62;
UPDATE creature_template SET ScriptName='' WHERE entry=100397;
UPDATE smart_scripts SET action_param1=123 WHERE entryorguid=100397 AND source_type=0 AND id=55 AND event_type=62;
$migration
SELECT 'custom-action',COUNT(*) FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=55 AND event_type=62 AND action_param1=123;
UPDATE smart_scripts SET action_param1=103481 WHERE entryorguid=100397 AND source_type=0 AND id=55 AND event_type=62;
UPDATE gossip_menu_option SET OptionBroadcastTextID=123 WHERE MenuID=19331 AND OptionID=0;
$migration
SELECT 'custom-menu',COUNT(*) FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=55 AND event_type=62;
UPDATE gossip_menu_option SET OptionBroadcastTextID=77067 WHERE MenuID=19331 AND OptionID=0;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES(22,56,100397,0,0,4,0,1,0,0,0,0,'','Shared custom flag/choice gate');
$migration
SELECT 'custom-shared-gate',COUNT(*) FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=55 AND event_type=62;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=56 AND SourceEntry=100397 AND SourceId=0;
UPDATE smart_scripts SET id=59 WHERE entryorguid=100397 AND source_type=0 AND id=58;
$migration
SELECT 'occupied-id',COUNT(*) FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=55 AND event_type=62;
UPDATE smart_scripts SET id=58 WHERE entryorguid=100397 AND source_type=0 AND id=59 AND event_type=60;
$rollback
SELECT 'rollback-choice',COUNT(*) FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=55 AND event_type=62;
SELECT 'rollback-rows',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId=15 AND SourceGroup=19331 AND SourceEntry=0) OR (SourceTypeOrReferenceId=22 AND SourceGroup=60 AND SourceEntry=100397);
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {$env:MYSQL_PWD=$connection[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Return admission SQL fixture failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("menu-count`t12","event-count`t12","same-conditions`t12","flag-unfiltered`t0","other-flags`t1","return-destination`t1","custom-lists`t2","custom-script`t1","custom-action`t1","custom-menu`t1","custom-shared-gate`t1","occupied-id`t1","rollback-choice`t1","rollback-rows`t0")){if($rows -notcontains $expected){throw "Return admission mismatch: $expected"}}
$actual=($rows | Where-Object {$_ -like "unchanged`t*"}) -replace '^unchanged',''
$saved=($rows | Where-Object {$_ -like "saved-unchanged`t*"}) -replace '^saved-unchanged',''
if($actual -ne $saved){throw 'Unrelated Vereesa script events changed'}
[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]@($rows | Where-Object {$_ -like "row`t*"} | ForEach-Object {$_.Substring(4)}))
[IO.File]::WriteAllLines("$out/links.tsv",[string[]]@($rows | Where-Object {$_ -like "actor-row`t*"} | ForEach-Object {$_.Substring(10)}))
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,$first);if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/RepeatGuard.inc",(Section $source '    if ((e.event.event_phase_mask && !IsInPhase(' '    ConditionList conds ='))
[IO.File]::WriteAllText("$out/FindLink.inc",(Section $source 'SmartScriptHolder SmartScript::FindLinkedEvent(' 'void SmartScript::OnUpdate('))
$first=$source.IndexOf('    if (e.link && e.link != e.event_id)');$last=$source.IndexOf('void SmartScript::InstallTemplate(', $first)
if($first -lt 0 -or $last -lt $first){throw 'Native LINK follow missing'}
$follow=$source.Substring($first,$last-$first);[IO.File]::WriteAllText("$out/FollowLink.inc",$follow.Substring(0,$follow.LastIndexOf('}')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/MarksmanshipReturnLinksTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/links.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native return LINK preservation failed'}
& "$PSScriptRoot/Test-TalonclawPickupAdmission.ps1" -MarksmanshipRowsPath "$out/conditions.tsv" -MarksmanshipReturn
& "$PSScriptRoot/Test-SurvivalArtifactCatalog.ps1"
$native=Get-Content "$repo/build-extractors/runtime-test/artifact-acquisition/survival-native.tsv"
foreach($expected in @("EFFECT`t205040`t0`t252`t0`t1317`t0`t0","EFFECT`t205040`t1`t90`t0`t102548`t0`t0","RETURN_TARGET`t0`t25`t17","RETURN_TARGET`t1`t25`t0")){if($native -notcontains $expected){throw "Native return spell mismatch: $expected"}}
if($VerifyReleaseBinary){$previousPython=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_marksmanship_report_use_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical SmartAI/condition bridge verification failed'}}finally{$env:PYTHONPATH=$previousPython}}
'PASS: repeated atomic return migration gates menu/cached choice on completed972/8 and actual ownership, preserving native flag/LINK/travel and custom definitions'
