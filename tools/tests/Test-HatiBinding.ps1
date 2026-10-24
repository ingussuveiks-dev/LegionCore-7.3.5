param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hati-binding"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_18_titanstrike_hati_binding.sql" -Raw
$scope='(SourceTypeOrReferenceId=22 AND SourceEntry=103154 AND SourceId=0 AND SourceGroup IN(5,6,7)) OR (SourceTypeOrReferenceId=17 AND SourceEntry=211967)'
$reset=@"
DELETE FROM conditions WHERE $scope;
DELETE FROM smart_scripts;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
DELETE FROM smart_scripts WHERE id=6;
UPDATE smart_scripts SET event_flags=1 WHERE id=4;
UPDATE smart_scripts SET link=0 WHERE id=5;
DELETE FROM creature_template;
INSERT INTO creature_template SELECT * FROM saved_actor;
DELETE FROM npc_spellclick_spells;
DELETE FROM spell_script_names;
"@
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=103154;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
INSERT INTO creature_template SELECT * FROM saved_actor;
CREATE TEMPORARY TABLE saved_click LIKE npc_spellclick_spells;
CREATE TEMPORARY TABLE npc_spellclick_spells LIKE saved_click;
CREATE TEMPORARY TABLE saved_bindings LIKE spell_script_names;
CREATE TEMPORARY TABLE spell_script_names LIKE saved_bindings;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=103154 AND source_type=0;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
$reset
$migration
SELECT 'FIRST';
SELECT * FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,ElseGroup,ConditionTypeOrReference,ConditionTarget;
$migration
SELECT 'SECOND';
SELECT * FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,ElseGroup,ConditionTypeOrReference,ConditionTarget;
SELECT 'END';
SELECT 'counts',SourceTypeOrReferenceId,SourceGroup,COUNT(*) FROM conditions GROUP BY SourceTypeOrReferenceId,SourceGroup ORDER BY SourceTypeOrReferenceId,SourceGroup;
SELECT 'row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=17 ORDER BY ElseGroup,ConditionTypeOrReference,ConditionTarget;
SELECT 'retry',COUNT(*) FROM smart_scripts WHERE id=4 AND event_flags=0 AND action_type=85 AND action_param1=211967 AND target_type=1;
SELECT 'hit-link',COUNT(*) FROM smart_scripts WHERE id=5 AND link=6 AND event_flags=1 AND event_type=8 AND action_param1=103154;
SELECT 'hide',COUNT(*) FROM smart_scripts WHERE id=6 AND event_type=61 AND action_type=83 AND action_param1=16777216 AND target_type=1;
SELECT 'same-predicates',COUNT(*) FROM conditions first JOIN conditions second ON first.ElseGroup=second.ElseGroup AND first.ConditionTypeOrReference=second.ConditionTypeOrReference AND first.ConditionTarget=second.ConditionTarget AND first.ConditionValue1=second.ConditionValue1 AND first.ConditionValue2=second.ConditionValue2 AND first.ConditionValue3=second.ConditionValue3 AND first.NegativeCondition=second.NegativeCondition WHERE first.SourceTypeOrReferenceId=17 AND second.SourceTypeOrReferenceId=22 AND second.SourceGroup IN(5,6);
"@
$variants=[ordered]@{
 'custom-script'="UPDATE creature_template SET ScriptName='custom_hati';"
 'custom-spell'='UPDATE smart_scripts SET action_param1=123 WHERE id=4;'
 'custom-hit'='UPDATE smart_scripts SET event_param1=123 WHERE id=5;'
 'custom-extra'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,event_chance,action_type,target_type,comment) VALUES(103154,0,20,64,100,1,1,'Custom event');"
 'custom-click'='INSERT INTO npc_spellclick_spells VALUES(103154,211967,1,0,1);'
 'custom-binding'="INSERT INTO spell_script_names VALUES(211967,'custom_bind');"
 'missing-actor'='DELETE FROM creature_template;'
 'custom-click-cooldown'='UPDATE smart_scripts SET event_param1=5000 WHERE id=4;'
 'custom-hit-school'='UPDATE smart_scripts SET event_param2=32 WHERE id=5;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`n$migration`nSELECT '$name',COUNT(*) FROM conditions;`n"}
$sql+=@"
$reset
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment) VALUES(22,5,103154,0,4,0,1,'Custom binding admission');
$migration
SELECT 'custom-condition',COUNT(*) FROM conditions;
SELECT 'custom-condition-kept',COUNT(*) FROM conditions WHERE Comment='Custom binding admission' AND ConditionTypeOrReference=4;
$reset
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment) VALUES(17,0,211967,0,4,0,1,'Custom cast admission');
$migration
SELECT 'custom-cast-condition',COUNT(*) FROM conditions;
SELECT 'custom-cast-kept',COUNT(*) FROM conditions WHERE Comment='Custom cast admission' AND ConditionTypeOrReference=4;
SELECT 'custom-no-partial',COUNT(*) FROM smart_scripts WHERE (id=4 AND event_flags=1) OR (id=5 AND link=0);
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Hati TEMP migration failed'}}finally{$env:MYSQL_PWD=$previous}
$rows | Set-Content -Encoding utf8 "$out/diagnostics.tsv"
$first=[Array]::IndexOf($rows,'FIRST');$second=[Array]::IndexOf($rows,'SECOND');$last=[Array]::IndexOf($rows,'END')
if($second-$first -ne 71 -or (Compare-Object $rows[($first+1)..($second-1)] $rows[($second+1)..($last-1)])){throw 'Hati migration not repeat-safe'}
foreach($expected in @("counts`t17`t0`t24","counts`t22`t5`t24","counts`t22`t6`t22","same-predicates`t46","retry`t1","hit-link`t1","hide`t1","custom-condition`t1","custom-condition-kept`t1","custom-cast-condition`t1","custom-cast-kept`t1","custom-no-partial`t2")+@($variants.Keys|ForEach-Object {"$_`t0"})){
 if($rows -notcontains $expected){throw "Hati fixture mismatch: $expected"}
}
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]]@($rows|Where-Object {$_ -like "row`t*"}|ForEach-Object {$_.Substring(4)}))
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$cases=(Section $source '        case CONDITION_AURA:' '        case CONDITION_ITEM:')+(Section $source '        case CONDITION_CLASS:' '        case CONDITION_RACE:')+(Section $source '        case CONDITION_QUESTREWARDED:' '        case CONDITION_QUEST_NONE:')+(Section $source '        case CONDITION_MAPID:' '        case CONDITION_AREAID:')+(Section $source '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:')+(Section $source '        case CONDITION_ALIVE:' '        case CONDITION_HP_VAL:')+(Section $source '        case CONDITION_OBJECT_ENTRY:' '        case CONDITION_TYPE_MASK:')
[IO.File]::WriteAllText("$out/Conditions.inc",$cases)
[IO.File]::WriteAllText("$out/ElseGroups.inc",(Section $source 'bool ConditionMgr::IsObjectMeetToConditionList(' 'bool ConditionMgr::IsObjectMeetToConditions(WorldObject* object,'))
[IO.File]::WriteAllText("$out/SmartConditions.inc",(Section $source 'bool ConditionMgr::IsObjectMeetingSmartEventConditions(' 'bool Condition::isLoaded() const'))
[IO.File]::WriteAllText("$out/SmartLookup.inc",(Section $source 'ConditionList ConditionMgr::GetConditionsForSmartEvent(' 'ConditionList ConditionMgr::GetConditionsForNpcVendorEvent('))
[IO.File]::WriteAllText("$out/CastLookup.inc",(Section $source 'ConditionList ConditionMgr::GetConditionsForNotGroupedEntry(' 'ConditionList ConditionMgr::GetConditionsForSpellClickEvent('))
$source=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
[IO.File]::WriteAllText("$out/CastConditions.inc",(Section $source '    ConditionSourceInfo condInfo = ConditionSourceInfo(m_caster);' "    // Don't check explicit target for passive spells"))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/EventDispatch.inc",(Section $source 'void SmartScript::ProcessEventsFor(' 'void SmartScript::ProcessAction('))
$head=Section $source 'void SmartScript::ProcessEvent(' '    switch (e.GetEventType())'
[IO.File]::WriteAllText("$out/EventHead.inc",$head.Substring($head.IndexOf('{')+1))
[IO.File]::WriteAllText("$out/ClickEvent.inc",(Section $source '        case SMART_EVENT_ON_SPELLCLICK:' '        case SMART_EVENT_RECEIVE_EMOTE:'))
[IO.File]::WriteAllText("$out/HitEvent.inc",(Section $source '        case SMART_EVENT_SPELLHIT:' '        case SMART_EVENT_OOC_LOS:'))
[IO.File]::WriteAllText("$out/RemoveFlag.inc",(Section $source '        case SMART_ACTION_REMOVE_NPC_FLAG:' '        case SMART_ACTION_CROSS_CAST:'))
$runOnce=[regex]::Match($source,'e\.runOnce = true;').Value
if(!$runOnce){throw 'Native run-once assignment absent'}
[IO.File]::WriteAllText("$out/RunOnce.inc",$runOnce)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/HatiBindingTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/rows.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Hati admission failed'}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TitanstrikeTheftCatalog.cpp -ProbeName TitanstrikeTheftCatalog -OutputFile "$out/native.tsv"
$native=Get-Content "$out/native.tsv"
foreach($expected in @("EFFECT`t211967`t0`t6`t4`t0`t0`t0`t25`t0","DURATION`t211967`t10000")){
 if($native -notcontains $expected){throw "Native Bind Spirit metadata mismatch: $expected"}
}
if($VerifyReleaseBinary){
 $readback=@'
SELECT 'live-scope',SourceTypeOrReferenceId,SourceGroup,COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId=22 AND SourceEntry=103154 AND SourceGroup IN(5,6)) OR (SourceTypeOrReferenceId=17 AND SourceEntry=211967) GROUP BY SourceTypeOrReferenceId,SourceGroup ORDER BY SourceTypeOrReferenceId,SourceGroup;
SELECT 'live-click-table',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=103154;
SELECT 'live-actions',COUNT(*) FROM smart_scripts WHERE entryorguid=103154 AND source_type=0 AND ((id=4 AND event_flags=0 AND action_type=85 AND action_param1=211967 AND target_type=1) OR (id=5 AND link=6 AND event_flags=1 AND event_type=8 AND action_param1=103154) OR (id=6 AND event_type=61 AND action_type=83 AND action_param1=16777216 AND target_type=1));
SELECT 'released',hash,state FROM updates WHERE name='2026_10_24_18_titanstrike_hati_binding.sql';
'@
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$live=@($readback|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Hati live readback failed'}}finally{$env:MYSQL_PWD=$previous}
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_18_titanstrike_hati_binding.sql" -Algorithm SHA1).Hash
 foreach($expected in @("live-scope`t17`t0`t24","live-scope`t22`t5`t24","live-scope`t22`t6`t22","live-click-table`t0","live-actions`t3","released`t$hash`tRELEASED")){if($live -notcontains $expected){throw "Live Hati mismatch: $expected"}}
 $live | Set-Content -Encoding utf8 "$out/live.tsv"
 $previousPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_hati_binding_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Deployed Hati consumers failed'}}finally{$env:PYTHONPATH=$previousPython}
}
'PASS: guarded repeat-safe Hati admission, native predicate and actual dispatcher/cast-check boundaries; failed-cast retry and successful-hit click cleanup use native data flags/LINK. Actual client binding remains pending.'
