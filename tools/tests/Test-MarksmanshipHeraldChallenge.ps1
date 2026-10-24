param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-herald-challenge"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_07_marksmanship_herald_challenge.sql" -Raw
$scope='SourceTypeOrReferenceId=22 AND SourceId=0 AND ((SourceEntry=100397 AND SourceGroup=26) OR (SourceEntry=100836 AND SourceGroup=1))'
$reset="DELETE FROM conditions WHERE $scope; DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart; DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actor;"
$sql=@"
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid IN(100397,100836);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry IN(100397,100836);
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
INSERT INTO creature_template SELECT * FROM saved_actor;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry IN(100397,100836);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions WHERE NOT ($scope);
SELECT 'before-smart',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT 'before-gates',conditions.* FROM conditions ORDER BY SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference;
$migration
SELECT 'first-guard',@marksman_herald_challenge;
$migration
SELECT 'after-smart',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT 'after-gates',conditions.* FROM conditions WHERE NOT ($scope) ORDER BY SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference;
SELECT 'count',COUNT(*) FROM conditions WHERE $scope;
SELECT 'row',entryorguid,id,link,event_type,event_phase_mask,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,target_type,target_param1,target_param2,target_param3 FROM smart_scripts WHERE source_type=0 AND ((entryorguid=100397 AND id IN(25,26)) OR (entryorguid=100836 AND id IN(0,1))) ORDER BY entryorguid,id;
SELECT 'gate',SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY SourceEntry,SourceGroup,ConditionTypeOrReference;
"@
$variants=@{
 'custom-script'="UPDATE creature_template SET ScriptName='custom_actor' WHERE entry=100397;"
 'custom-ai'="UPDATE creature_template SET AIName='' WHERE entry=100836;"
 'custom-range'='UPDATE smart_scripts SET target_param2=75 WHERE entryorguid=100397 AND id=25;'
 'custom-repeat'='UPDATE smart_scripts SET event_flags=0 WHERE entryorguid=100397 AND id=25;'
 'custom-difficulty'="UPDATE smart_scripts SET Difficulties='1' WHERE entryorguid=100397 AND id=25;"
 'custom-link'='UPDATE smart_scripts SET link=59 WHERE entryorguid=100397 AND id=25;'
 'custom-signal'='UPDATE smart_scripts SET action_param2=2 WHERE entryorguid=100397 AND id=25;'
 'custom-speech'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=100836 AND id=0;'
 'custom-duration'='UPDATE smart_scripts SET action_param2=4000 WHERE entryorguid=100836 AND id=0;'
 'custom-text-over'='UPDATE smart_scripts SET action_param2=7 WHERE entryorguid=100836 AND id=1;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`n$migration`nSELECT '$name',COUNT(*) FROM conditions WHERE $scope;`n"}
foreach($entry in @(100397,100836)){
 $group=if($entry -eq 100397){26}else{1}
 $sql+="$reset`nINSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment) VALUES(22,$group,$entry,0,1,4,1,1489,'Custom Herald admission');`n$migration`nSELECT 'custom-gate-$entry',COUNT(*) FROM conditions WHERE $scope;`nSELECT 'custom-preserved-$entry',COUNT(*) FROM conditions WHERE Comment='Custom Herald admission';`nDELETE FROM conditions WHERE Comment='Custom Herald admission';`n"
}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Herald temporary SQL fixture failed'}}finally{$env:MYSQL_PWD=$previous}
[IO.File]::WriteAllLines("$out/fixture-diagnostics.tsv",[string[]]$rows)
foreach($expected in @("count`t3","custom-gate-100397`t1","custom-gate-100836`t1","custom-preserved-100397`t1","custom-preserved-100836`t1")+@($variants.Keys | ForEach-Object {"$_`t0"})){if($rows -notcontains $expected){throw "Herald mismatch: $expected"}}
foreach($kind in @('smart','gates')){
 $before=@($rows | Where-Object {$_ -like "before-$kind`t*"} | ForEach-Object {$_.Substring(8+$kind.Length)})
 $after=@($rows | Where-Object {$_ -like "after-$kind`t*"} | ForEach-Object {$_.Substring(7+$kind.Length)})
 if(Compare-Object $before $after){throw "Unrelated Herald $kind fields changed"}
}
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]]@($rows | Where-Object {$_ -like "row`t*"} | ForEach-Object {$_.Substring(4)}))
[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]@($rows | Where-Object {$_ -like "gate`t*"} | ForEach-Object {$_.Substring(5)}))
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$sections=@{
 'HeraldConditions'=@('        case CONDITION_NEAR_CREATURE:','        case CONDITION_NEAR_GAMEOBJECT:')
 'HeraldScenario'=@('        case CONDITION_SCENARION_STEP:','        case CONDITION_REALM_ACHIEVEMENT:')
 'ElseGroups'=@('bool ConditionMgr::IsObjectMeetToConditionList(','bool ConditionMgr::IsObjectMeetToConditions(WorldObject* object,')
 'SmartEventConditions'=@('bool ConditionMgr::IsObjectMeetingSmartEventConditions(','bool Condition::isLoaded() const')
 'EventConditions'=@('ConditionList ConditionMgr::GetConditionsForSmartEvent(','ConditionList ConditionMgr::GetConditionsForNpcVendorEvent(')
}
foreach($name in $sections.Keys){[IO.File]::WriteAllText("$out/$name.inc",(Section $source $sections[$name][0] $sections[$name][1]))}
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$sections=@{
 'Dispatch'=@('void SmartScript::ProcessEventsFor(','void SmartScript::ProcessAction(')
 'RepeatGuard'=@('    if ((e.event.event_phase_mask && !IsInPhase(','    ConditionList conds =')
 'EventGate'=@('    ConditionList conds = sConditionMgr->GetConditionsForSmartEvent','    switch (e.GetEventType())')
 'ActionStart'=@('    e.runOnce = true;','    if (Unit* tempInvoker = GetLastInvoker())')
 'TimedUpdate'=@('        case SMART_EVENT_UPDATE:','        case SMART_EVENT_UPDATE_OOC:')
 'DataEvent'=@('        case SMART_EVENT_DATA_SET:','        case SMART_EVENT_PASSENGER_REMOVED:')
 'TextOver'=@('        case SMART_EVENT_TEXT_OVER:','        case SMART_EVENT_DATA_SET:')
 'SetData'=@('        case SMART_ACTION_SET_DATA:','        case SMART_ACTION_MOVE_FORWARD:')
 'Talk'=@('        case SMART_ACTION_TALK:','        case SMART_ACTION_SIMPLE_TALK:')
 'ClosestCreature'=@('        case SMART_TARGET_CLOSEST_CREATURE:','        case SMART_TARGET_CLOSEST_GAMEOBJECT:')
 'UpdateTimer'=@('void SmartScript::UpdateTimer(','bool SmartScript::CheckTimer(')
 'RecalcTimer'=@('void SmartScript::RecalcTimer(','void SmartScript::UpdateTimer(')
 'TextTimer'=@('    if (mUseTextTimer && me)','void SmartScript::FillScript(')
}
foreach($name in $sections.Keys){
 $section=Section $source $sections[$name][0] $sections[$name][1]
 if($name -eq 'TextTimer'){$section=$section.TrimEnd();if(!$section.EndsWith('}')){throw 'Native text timer boundary changed'};$section=$section.Substring(0,$section.Length-1)}
 [IO.File]::WriteAllText("$out/$name.inc",$section)
}
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
[IO.File]::WriteAllText("$out/AiSetData.inc",(Section $source 'void SmartAI::SetData(' 'void SmartAI::SetGUID('))
$source=Get-Content "$repo/src/server/game/AI/ScriptedAI/ScriptedCreature.h" -Raw
[IO.File]::WriteAllText("$out/NearestHelper.inc",(Section $source 'inline Creature* GetClosestCreatureWithEntry(' 'inline GameObject* GetClosestGameObjectWithEntry('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/MarksmanshipHeraldChallengeTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/rows.tsv`" `"$out/conditions.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Herald challenge regression failed'}
if($VerifyReleaseBinary){
 $previousPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_marksmanship_herald_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Herald PE verification failed'}}finally{$env:PYTHONPATH=$previousPython}
}
'PASS: three native conditions preserve all SmartAI/speech fields and custom admissions; executable sender/receiver timer regression retains challenge until its stage and living actor'
