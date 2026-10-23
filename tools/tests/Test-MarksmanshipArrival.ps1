param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-arrival"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_23_24_marksmanship_arrival.sql" -Raw
$reset="DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=100397 AND SourceId=0; INSERT INTO conditions SELECT * FROM saved_conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=100397 AND SourceId=0 AND ConditionTypeOrReference=44; UPDATE smart_scripts SET target_type=7 WHERE entryorguid=100397 AND source_type=0 AND id=0;"
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=100397;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
INSERT INTO creature_template SELECT * FROM saved_actor;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=100397 AND source_type=0;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100397;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
$reset
SELECT 'before',smart_scripts.* FROM smart_scripts WHERE id<>0 ORDER BY id;
$migration
$migration
SELECT 'after',smart_scripts.* FROM smart_scripts WHERE id<>0 ORDER BY id;
SELECT 'row',id,link,event_type,event_flags,event_param1,event_param2,action_type,action_param1,action_param2,action_param3,action_param4,target_type FROM smart_scripts WHERE id IN(0,1,3,4,57) ORDER BY id;
SELECT 'gate',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=100397 AND SourceId=0 ORDER BY ConditionTypeOrReference;
SELECT 'count',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=100397 AND SourceId=0;
$reset
UPDATE creature_template SET ScriptName='custom_actor' WHERE entry=100397;
$migration
SELECT 'custom-script',target_type FROM smart_scripts WHERE id=0;
UPDATE creature_template SET ScriptName='' WHERE entry=100397;
UPDATE smart_scripts SET target_type=21 WHERE id=0;
$migration
SELECT 'custom-target',target_type FROM smart_scripts WHERE id=0;
$reset
UPDATE conditions SET ConditionTarget=0 WHERE SourceGroup=1;
$migration
SELECT 'custom-stage',target_type FROM smart_scripts WHERE id=0;
$reset
UPDATE smart_scripts SET event_param2=416 WHERE id=3;
$migration
SELECT 'custom-timer',target_type FROM smart_scripts WHERE id=0;
UPDATE smart_scripts SET event_param2=100397 WHERE id=3;
$reset
UPDATE smart_scripts SET action_param1=999 WHERE id=57;
$migration
SELECT 'custom-credit',target_type FROM smart_scripts WHERE id=0;
UPDATE smart_scripts SET action_param1=102129 WHERE id=57;
$reset
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES(22,1,100397,0,1,4,0,1,0,0,0,0,'','Custom arrival admission');
$migration
SELECT 'custom-gate',target_type FROM smart_scripts WHERE id=0;
SELECT 'custom-gate-preserved',COUNT(*) FROM conditions WHERE SourceGroup=1 AND Comment='Custom arrival admission';
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Arrival temporary SQL fixture failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("count`t2","custom-script`t7","custom-target`t21","custom-stage`t7","custom-timer`t7","custom-credit`t7","custom-gate`t7","custom-gate-preserved`t1")){if($rows -notcontains $expected){throw "Arrival mismatch: $expected"}}
$before=@($rows | Where-Object {$_ -like "before`t*"} | ForEach-Object {$_.Substring(7)})
$after=@($rows | Where-Object {$_ -like "after`t*"} | ForEach-Object {$_.Substring(6)})
if(Compare-Object $before $after){throw 'Unrelated actor fields changed'}
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]]@($rows | Where-Object {$_ -like "row`t*"} | ForEach-Object {$_.Substring(4)}))
[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]@($rows | Where-Object {$_ -like "gate`t*"} | ForEach-Object {$_.Substring(5)}))
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/Conditions.inc",(Section $source '        case CONDITION_TYPE_MASK:' '        case CONDITION_RELATION_TO:')+(Section $source '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:'))
[IO.File]::WriteAllText("$out/ElseGroups.inc",(Section $source 'bool ConditionMgr::IsObjectMeetToConditionList(' 'bool ConditionMgr::IsObjectMeetToConditions(WorldObject* object,'))
[IO.File]::WriteAllText("$out/SmartEventConditions.inc",(Section $source 'bool ConditionMgr::IsObjectMeetingSmartEventConditions(' 'bool Condition::isLoaded() const'))
[IO.File]::WriteAllText("$out/EventConditions.inc",(Section $source 'ConditionList ConditionMgr::GetConditionsForSmartEvent(' 'ConditionList ConditionMgr::GetConditionsForNpcVendorEvent('))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$sections=@{
 'Dispatch'=@('void SmartScript::ProcessEventsFor(','void SmartScript::ProcessAction(')
 'RepeatGuard'=@('    if ((e.event.event_phase_mask && !IsInPhase(','    ConditionList conds =')
 'EventGate'=@('    ConditionList conds = sConditionMgr->GetConditionsForSmartEvent','    switch (e.GetEventType())')
 'Los'=@('        case SMART_EVENT_OOC_LOS:','        case SMART_EVENT_IC_LOS:')
 'TextOver'=@('        case SMART_EVENT_TEXT_OVER:','        case SMART_EVENT_DATA_SET:')
 'ActionStart'=@('    e.runOnce = true;','    if (Unit* tempInvoker = GetLastInvoker())')
 'Talk'=@('        case SMART_ACTION_TALK:','        case SMART_ACTION_SIMPLE_TALK:')
 'Waypoint'=@('        case SMART_ACTION_WP_START:','        case SMART_ACTION_WP_PAUSE:')
 'FindLink'=@('SmartScriptHolder SmartScript::FindLinkedEvent(','void SmartScript::OnUpdate(')
 'LinkTail'=@('    if (e.link && e.link != e.event_id)','void SmartScript::InstallTemplate(')
}
foreach($name in $sections.Keys){[IO.File]::WriteAllText("$out/$name.inc",(Section $source $sections[$name][0] $sections[$name][1]))}
$timer=Section $source '    if (mUseTextTimer && me)' 'void SmartScript::FillScript('
[IO.File]::WriteAllText("$out/TextTimer.inc",$timer.Substring(0,$timer.LastIndexOf('}')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/MarksmanshipArrivalTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/rows.tsv`" `"$out/conditions.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native arrival regression failed'}
if($VerifyReleaseBinary){$previousPython=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_marksmanship_arrival_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical arrival PE verification failed'}}finally{$env:PYTHONPATH=$previousPython}}
'PASS: repeated scoped arrival migration retains native base-actor scenario0 gate/10yd range, adds Player-only admission/self speaker, and preserves unrelated/custom actor data'
