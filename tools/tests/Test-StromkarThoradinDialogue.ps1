param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-thoradin-dialogue"
New-Item -ItemType Directory -Force $out|Out-Null
$migrationPath="$repo/sql/updates/world/2026_10_24_53_stromkar_thoradin_dialogue_order.sql"
$hash=(Get-FileHash $migrationPath -Algorithm SHA1).Hash
$migration=Get-Content $migrationPath -Raw
$ready="entryorguid=104307 AND source_type=0 AND id=5 AND link=0 AND event_type=52 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1 AND event_param1=1 AND event_param2=104307 AND event_param3=0 AND event_param4=0 AND action_type=45 AND action_param1=2 AND action_param2=1 AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0 AND target_type=1 AND target_param1=0 AND target_param2=0 AND target_param3=0 AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0"
$reset=@'
DELETE FROM creature_template;INSERT INTO creature_template SELECT * FROM saved_actor;
DELETE FROM smart_scripts;INSERT INTO smart_scripts SELECT * FROM saved_smart;
DELETE FROM conditions;INSERT INTO conditions SELECT * FROM saved_conditions;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry IN(104276,104307);
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid IN(104276,104307,103151);
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry IN(104276,104307);
UPDATE saved_actor SET ScriptName='' WHERE entry=104307 AND ScriptName='npc_stromkar_thoradin' AND EXISTS(SELECT 1 FROM updates WHERE name='2026_10_24_53_stromkar_thoradin_dialogue_order.sql' AND hash='$hash' AND state='RELEASED');
DELETE FROM saved_smart WHERE $ready AND EXISTS(SELECT 1 FROM updates WHERE name='2026_10_24_53_stromkar_thoradin_dialogue_order.sql' AND hash='$hash' AND state='RELEASED');
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
$reset
SELECT 'old-before',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT 'conditions-before',conditions.* FROM conditions ORDER BY SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference;
$migration
SELECT 'first',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;SELECT 'binding',ScriptName FROM creature_template WHERE entry=104307;
SELECT 'row',id,link,event_type,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,target_type FROM smart_scripts WHERE entryorguid=104307 ORDER BY id;
$migration
SELECT 'second',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT 'old-after',smart_scripts.* FROM smart_scripts WHERE NOT($ready) ORDER BY entryorguid,source_type,id;
SELECT 'conditions-after',conditions.* FROM conditions ORDER BY SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference;
"@
$variants=[ordered]@{
 'custom-script'="UPDATE creature_template SET ScriptName='custom' WHERE entry=104307;"
 'custom-ai'="UPDATE creature_template SET AIName='' WHERE entry=104307;"
 'custom-flags'='UPDATE creature_template SET npcflag=1 WHERE entry=104307;'
 'custom-owner'="UPDATE creature_template SET ScriptName='custom' WHERE entry=104276;"
 'custom-intro'='UPDATE smart_scripts SET action_param2=3000 WHERE entryorguid=104307 AND id=0;'
 'custom-prompt'='UPDATE smart_scripts SET event_param2=103144 WHERE entryorguid=104307 AND id=1;'
 'custom-path'='UPDATE smart_scripts SET action_param2=103144 WHERE entryorguid=104307 AND id=2;'
 'custom-finale'='UPDATE smart_scripts SET event_flags=0 WHERE entryorguid=104307 AND id=3;'
 'custom-farewell'='UPDATE smart_scripts SET link=99 WHERE entryorguid=104307 AND id=4;'
 'occupied-row'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,comment)VALUES(104307,0,5,60,1,'custom');"
 'custom-gate'='INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1)VALUES(22,1,104307,36,0);'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;SELECT '$name-actor-before',creature_template.* FROM creature_template ORDER BY entry;`n$migration`nSELECT '$name-after',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;SELECT '$name-actor-after',creature_template.* FROM creature_template ORDER BY entry;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Dialogue TEMP query failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/fixture-diagnostics.tsv",[string[]]$rows)
function Snapshot([string]$label){[string[]]@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
function Signature([string]$label){[string]::Join("`n",[string[]](Snapshot $label))}
foreach($pair in @(@('first','second'),@('old-before','old-after'),@('conditions-before','conditions-after'))){if((Signature $pair[0]) -cne (Signature $pair[1])){throw "Dialogue changed $pair"}}
foreach($name in $variants.Keys){foreach($suffix in @('','-actor')){if((Signature "$name$suffix-before") -cne (Signature "$name$suffix-after")){throw "Custom dialogue overwritten: $name$suffix"}}}
if($rows -notcontains "binding`tnpc_stromkar_thoradin" -or @(Snapshot 'row').Count -ne 6){throw 'Expected bound ghost and six exact dialogue rows'}
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]](Snapshot 'row'))
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
if(!$source.Contains('RegisterCreatureAI(npc_stromkar_thoradin);')){throw 'Missing ghost registration'}
[IO.File]::WriteAllText("$out/Thoradin.inc",(Section $source 'struct npc_stromkar_thoradin : SmartAI' 'void AddSC_artifact_acquisition()'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
[IO.File]::WriteAllText("$out/Summoned.inc",(Section $source 'void SmartAI::IsSummonedBy(' 'void SmartAI::DamageDealt('))
[IO.File]::WriteAllText("$out/SetData.inc",(Section $source 'void SmartAI::SetData(' 'void SmartAI::SetGUID('))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/ProcessEvents.inc",(Section $source 'void SmartScript::ProcessEventsFor(' 'void SmartScript::ProcessAction('))
[IO.File]::WriteAllText("$out/FindLinked.inc",(Section $source 'SmartScriptHolder SmartScript::FindLinkedEvent(' 'void SmartScript::OnUpdate('))
[IO.File]::WriteAllText("$out/OnReset.inc",(Section $source 'void SmartScript::OnReset()' 'void SmartScript::ResetBaseObject()'))
[IO.File]::WriteAllText("$out/TextTimer.inc",(Section $source '    if (mUseTextTimer && me)' 'void SmartScript::FillScript(').TrimEnd().TrimEnd('}'))
$event=(Section $source 'void SmartScript::ProcessEvent(' '        //called from Update tick')
$event+=(Section $source '        case SMART_EVENT_JUST_SUMMONED:' '        case SMART_EVENT_IS_BEHIND_TARGET:')
$event+=(Section $source '        case SMART_EVENT_TEXT_OVER:' '        case SMART_EVENT_PASSENGER_REMOVED:')
$event+="        default: assert(false);`n    }`n}`n"
$event=$event.Replace("        case SMART_EVENT_JUST_CREATED:`r`n",'').Replace("        case SMART_EVENT_GOSSIP_HELLO:`r`n",'').Replace("        case SMART_EVENT_FOLLOW_COMPLETED:`r`n",'')
$event=$event.Replace("        case SMART_EVENT_JUST_CREATED:`n",'').Replace("        case SMART_EVENT_GOSSIP_HELLO:`n",'').Replace("        case SMART_EVENT_FOLLOW_COMPLETED:`n",'')
[IO.File]::WriteAllText("$out/ProcessEvent.inc",$event)
$action=(Section $source 'void SmartScript::ProcessAction(' '        case SMART_ACTION_SIMPLE_TALK:')
$action+=(Section $source '        case SMART_ACTION_SET_DATA:' '        case SMART_ACTION_MOVE_FORWARD:')
$action+="        case SMART_ACTION_WP_START: ++pathStarts;break;`n        default: assert(false);`n    }`n"
$action+=(Section $source '    if (e.link && e.link != e.event_id)' 'void SmartScript::InstallTemplate(')
[IO.File]::WriteAllText("$out/ProcessAction.inc",$action)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($baseline in @($true,$false)){
 $name=if($baseline){'baseline'}else{'test'};$definition=if($baseline){'/DBASELINE'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/StromkarThoradinDialogueTest.cpp`" /Fe`"$out/$name.exe`" /Fo`"$out/$name.obj`"`nif errorlevel 1 exit /b 1`n`"$out/$name.exe`" `"$out/rows.tsv`"`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw "Native Thoradin dialogue $name failed"}
}
if($VerifyReleaseBinary){
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_53_stromkar_thoradin_dialogue_order.sql';SELECT 'binding',ScriptName FROM creature_template WHERE entry=104307 AND AIName='SmartAI';SELECT 'ready',COUNT(*) FROM smart_scripts WHERE $ready;"
 if($live -notcontains "release`t$hash`tRELEASED" -or $live -notcontains "binding`tnpc_stromkar_thoradin" -or $live -notcontains "ready`t1"){throw 'Live53 dialogue hash/binding differs'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_stromkar_thoradin_dialogue_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Thoradin dialogue failed'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: native text-timer baseline loses line1 on an early final request; scoped queued handoff preserves four existing lines, native durations/waypoint and exactly-once farewell without blocking gameplay or inventing visuals. Full client/audio/departure and actual boss hit remain separate.'
