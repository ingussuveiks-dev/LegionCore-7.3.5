param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-claim-dialogue"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_23_23_marksmanship_claim_dialogue.sql" -Raw
$reset="DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100397 AND SourceGroup=53; UPDATE smart_scripts SET target_type=21,target_param1=50 WHERE entryorguid=100397 AND source_type=0 AND id IN(52,53);"
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
$migration
$migration
SELECT 'gate-count',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100397 AND SourceGroup=53 AND SourceId=0;
SELECT 'same-gates',COUNT(*) FROM conditions speech JOIN conditions exit_gate ON speech.ElseGroup=exit_gate.ElseGroup AND speech.ConditionTypeOrReference=exit_gate.ConditionTypeOrReference AND speech.ConditionTarget=exit_gate.ConditionTarget AND speech.ConditionValue1=exit_gate.ConditionValue1 AND speech.ConditionValue2=exit_gate.ConditionValue2 AND speech.ConditionValue3=exit_gate.ConditionValue3 AND speech.NegativeCondition=exit_gate.NegativeCondition WHERE speech.SourceTypeOrReferenceId=22 AND speech.SourceEntry=100397 AND speech.SourceGroup=53 AND exit_gate.SourceTypeOrReferenceId=22 AND exit_gate.SourceEntry=100397 AND exit_gate.SourceGroup=60;
SELECT 'row',id,link,event_type,event_flags,event_param1,event_param2,action_type,action_param1,action_param2,target_type,target_param1,event_chance,event_phase_mask FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id IN(52,53,54,55) ORDER BY id;
SELECT 'gate',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100397 AND SourceGroup=53 AND SourceId=0 ORDER BY ElseGroup,ConditionTypeOrReference;
SELECT 'unrelated',COUNT(*) FROM smart_scripts current JOIN saved_smart old ON current.entryorguid=old.entryorguid AND current.source_type=old.source_type AND current.id=old.id AND current.event_type=old.event_type AND current.link=old.link WHERE current.id NOT IN(52,53) AND current.action_type=old.action_type AND current.action_param1=old.action_param1 AND current.action_param2=old.action_param2 AND current.target_type=old.target_type AND current.event_flags=old.event_flags;
SELECT 'saved-unrelated',COUNT(*) FROM saved_smart WHERE id NOT IN(52,53);
$reset
UPDATE creature_template SET ScriptName='custom_actor' WHERE entry=100397;
$migration
SELECT 'custom-script',COUNT(*) FROM smart_scripts WHERE id IN(52,53) AND target_type=21;
UPDATE creature_template SET ScriptName='' WHERE entry=100397;
UPDATE smart_scripts SET event_chance=50 WHERE id=52;
$migration
SELECT 'custom-chance',COUNT(*) FROM smart_scripts WHERE id IN(52,53) AND target_type=21;
UPDATE smart_scripts SET event_chance=100 WHERE id=52;
UPDATE smart_scripts SET action_param2=1 WHERE id=53;
$migration
SELECT 'custom-position',COUNT(*) FROM smart_scripts WHERE id IN(52,53) AND target_type=21;
UPDATE smart_scripts SET action_param2=0 WHERE id=53;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES(22,53,100397,0,0,4,0,1,0,0,0,0,'','Custom dialogue admission');
$migration
SELECT 'custom-gate',COUNT(*) FROM smart_scripts WHERE id IN(52,53) AND target_type=21;
SELECT 'custom-gate-preserved',COUNT(*) FROM conditions WHERE SourceGroup=53 AND Comment='Custom dialogue admission';
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Claim dialogue temporary SQL fixture failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("gate-count`t12","same-gates`t12","custom-script`t2","custom-chance`t2","custom-position`t2","custom-gate`t2","custom-gate-preserved`t1")){if($rows -notcontains $expected){throw "Claim dialogue mismatch: $expected"}}
if((($rows | Where-Object {$_ -like "unrelated`t*"}) -replace '^unrelated','') -ne (($rows | Where-Object {$_ -like "saved-unrelated`t*"}) -replace '^saved-unrelated','')){throw 'Unrelated actor actions changed'}
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]]@($rows | Where-Object {$_ -like "row`t*"} | ForEach-Object {$_.Substring(4)}))
[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]@($rows | Where-Object {$_ -like "gate`t*"} | ForEach-Object {$_.Substring(5)}))
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
$first=$source.LastIndexOf('namespace MarksmanshipClaim');$last=$source.IndexOf('void AddSC_artifact_acquisition()',$first)
[IO.File]::WriteAllText("$out/Claim.inc",$source.Substring($first,$last-$first))
[IO.File]::WriteAllText("$out/Active.inc",(Section $source 'bool Active(Player*' 'bool Use(Player*'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.h" -Raw
[IO.File]::WriteAllText("$out/Readers.inc",(Section $source '        bool IsTextPending(' '    private:'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/Dispatch.inc",(Section $source 'void SmartScript::ProcessEventsFor(' 'void SmartScript::ProcessAction('))
[IO.File]::WriteAllText("$out/RepeatGuard.inc",(Section $source '    if ((e.event.event_phase_mask && !IsInPhase(' '    ConditionList conds ='))
[IO.File]::WriteAllText("$out/TextOver.inc",(Section $source '        case SMART_EVENT_TEXT_OVER:' '        case SMART_EVENT_DATA_SET:'))
[IO.File]::WriteAllText("$out/ActionStart.inc",(Section $source '    e.runOnce = true;' '    if (Unit* tempInvoker = GetLastInvoker())'))
[IO.File]::WriteAllText("$out/Talk.inc",(Section $source '        case SMART_ACTION_TALK:' '        case SMART_ACTION_SIMPLE_TALK:'))
[IO.File]::WriteAllText("$out/Conversation.inc",(Section $source '        case SMART_ACTION_SUMMON_CONVERSATION:' '        case SMART_ACTION_SUMMON_ADD_PLR_PERSONNAL_VISIBILE:'))
[IO.File]::WriteAllText("$out/Flag.inc",(Section $source '        case SMART_ACTION_SET_NPC_FLAG:' '        case SMART_ACTION_ADD_NPC_FLAG:'))
[IO.File]::WriteAllText("$out/InvokerTarget.inc",(Section $source '        case SMART_TARGET_NONE:' '        case SMART_TARGET_ACTION_INVOKER_VEHICLE:'))
[IO.File]::WriteAllText("$out/FindLink.inc",(Section $source 'SmartScriptHolder SmartScript::FindLinkedEvent(' 'void SmartScript::OnUpdate('))
[IO.File]::WriteAllText("$out/LinkTail.inc",(Section $source '    if (e.link && e.link != e.event_id)' 'void SmartScript::InstallTemplate('))
$first=$source.IndexOf('    if (mUseTextTimer && me)');$last=$source.IndexOf('void SmartScript::FillScript(',$first)
$timer=$source.Substring($first,$last-$first);[IO.File]::WriteAllText("$out/TextTimer.inc",$timer.Substring(0,$timer.LastIndexOf('}')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/MarksmanshipClaimDialogueTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/rows.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native claim dialogue regression failed'}
& "$PSScriptRoot/Test-TalonclawPickupAdmission.ps1" -MarksmanshipRowsPath "$out/conditions.tsv" -MarksmanshipDialogue
& "$PSScriptRoot/Test-MarksmanshipPresentation.ps1"
if($VerifyReleaseBinary){$previousPython=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_marksmanship_claim_dialogue_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical claim dialogue PE verification failed'}}finally{$env:PYTHONPATH=$previousPython}}
'PASS: repeated claim-dialogue migration gates completed owner52 -> source53, preserves custom definitions and retains original texts/conversation/return flags'
