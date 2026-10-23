param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-orestes"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_01_marksmanship_orestes_discovery.sql" -Raw
$reset="DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=4 AND SourceEntry=100398 AND SourceId=0; UPDATE smart_scripts SET target_type=21,target_param1=100 WHERE entryorguid=100398 AND source_type=0 AND id=3;"
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=100398;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
INSERT INTO creature_template SELECT * FROM saved_actor;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid IN(100398,100749);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100398;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
$reset
UPDATE smart_scripts SET target_type=7,target_param1=0 WHERE entryorguid=100398 AND source_type=0 AND id=3;
SELECT 'before',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;
$reset
$migration
$migration
SELECT 'after',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT 'row',id,link,event_type,event_flags,event_param1,event_param2,action_type,action_param1,action_param2,action_param3,action_param4,target_type,target_param1 FROM smart_scripts WHERE entryorguid=100398 AND source_type=0 AND id IN(3,4) ORDER BY id;
SELECT 'gate',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=4 AND SourceEntry=100398 AND SourceId=0 ORDER BY ConditionTypeOrReference;
SELECT 'count',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=4 AND SourceEntry=100398 AND SourceId=0;
SELECT 'creature',entry,id,creatureId,creatureGuid,unk1,unk2,duration FROM conversation_creature WHERE entry=1073 ORDER BY id;
SELECT 'line',entry,id,idx,textId,unk1,unk2 FROM conversation_data WHERE entry=1073 ORDER BY idx;
$reset
UPDATE creature_template SET ScriptName='custom_actor';
$migration
SELECT 'custom-script',target_type FROM smart_scripts WHERE entryorguid=100398 AND source_type=0 AND id=3;
UPDATE creature_template SET ScriptName='';
$reset
UPDATE smart_scripts SET target_param1=50 WHERE entryorguid=100398 AND source_type=0 AND id=3;
$migration
SELECT 'custom-range',target_param1 FROM smart_scripts WHERE entryorguid=100398 AND source_type=0 AND id=3;
$reset
UPDATE smart_scripts SET action_param1=200832 WHERE entryorguid=100398 AND source_type=0 AND id=4;
$migration
SELECT 'custom-speech',target_type FROM smart_scripts WHERE entryorguid=100398 AND source_type=0 AND id=3;
UPDATE smart_scripts SET action_param1=200848 WHERE entryorguid=100398 AND source_type=0 AND id=4;
$reset
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES(22,4,100398,0,1,4,0,1,0,0,0,0,'','Custom discovery admission');
$migration
SELECT 'custom-gate',target_type FROM smart_scripts WHERE entryorguid=100398 AND source_type=0 AND id=3;
SELECT 'custom-gate-preserved',COUNT(*) FROM conditions WHERE SourceGroup=4 AND Comment='Custom discovery admission';
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Orestes temporary SQL fixture failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("count`t2","custom-script`t21","custom-range`t50","custom-speech`t21","custom-gate`t21","custom-gate-preserved`t1")){if($rows -notcontains $expected){throw "Orestes mismatch: $expected"}}
$before=@($rows | Where-Object {$_ -like "before`t*"} | ForEach-Object {$_.Substring(7)})
$after=@($rows | Where-Object {$_ -like "after`t*"} | ForEach-Object {$_.Substring(6)})
if(Compare-Object $before $after){throw 'Unrelated Orestes/Torvis fields changed'}
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]]@($rows | Where-Object {$_ -like "row`t*"} | ForEach-Object {$_.Substring(4)}))
[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]@($rows | Where-Object {$_ -like "gate`t*"} | ForEach-Object {$_.Substring(5)}))
[IO.File]::WriteAllLines("$out/conversation.tsv",[string[]]@($rows | Where-Object {$_ -like "creature`t*" -or $_ -like "line`t*"}))
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
 'ActionStart'=@('    e.runOnce = true;','    if (Unit* tempInvoker = GetLastInvoker())')
 'FindLink'=@('SmartScriptHolder SmartScript::FindLinkedEvent(','void SmartScript::OnUpdate(')
 'LinkTail'=@('    if (e.link && e.link != e.event_id)','void SmartScript::InstallTemplate(')
 'CriteriaAction'=@('        case SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA:','        case SMART_ACTION_SUMMON_CONVERSATION:')
 'InvokerCast'=@('        case SMART_ACTION_INVOKER_CAST:','        case SMART_ACTION_ADD_AURA:')
 'CreditHelpers'=@('namespace','SmartScript::SmartScript()')
 'InvokerTarget'=@('        case SMART_TARGET_ACTION_INVOKER:','        case SMART_TARGET_ACTION_INVOKER_VEHICLE:')
}
foreach($name in $sections.Keys){[IO.File]::WriteAllText("$out/$name.inc",(Section $source $sections[$name][0] $sections[$name][1]))}
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
[IO.File]::WriteAllText("$out/CastFlags.inc",(Section $source 'enum SmartCastFlags' '// one line in DB'))
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
[IO.File]::WriteAllText("$out/OwnerPlayer.inc",(Section $source 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const' 'Player* Unit::GetAffectingPlayer() const'))
$source=Get-Content "$repo/src/server/game/Globals/ConversationData.h" -Raw
[IO.File]::WriteAllText("$out/ConversationStructs.inc",(Section $source 'struct ConversationActor' 'typedef std::unordered_map'))
$source=Get-Content "$repo/src/server/game/Entities/Conversation/Conversation.cpp" -Raw
[IO.File]::WriteAllText("$out/ConversationData.inc",$source.Substring($source.IndexOf('bool Conversation::GenerateDynamicFieldData(')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/MarksmanshipOrestesTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/rows.tsv`" `"$out/conditions.tsv`" `"$out/conversation.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Orestes regression failed'}
if($VerifyReleaseBinary){& "$PSScriptRoot/Test-MarksmanshipArrival.ps1" -VerifyReleaseBinary;& "$PSScriptRoot/Test-ScriptPetCredit.ps1" -VerifyReleaseBinary}
'PASS: repeated scoped discovery migration preserves native5yd LOS/link200848 and every other Orestes/Torvis row; custom definitions retained'
