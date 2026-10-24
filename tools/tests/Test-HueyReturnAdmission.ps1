param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/huey-return-admission"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_16_titanstrike_huey_return_admission.sql" -Raw
$click="SourceTypeOrReferenceId=18 AND SourceGroup=106854 AND SourceEntry=212060 AND SourceId=0"
$root="SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=106854 AND SourceId=0"
$outgoing="SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=106854 AND SourceId=0"
$snapshot=@"
SELECT 'click-row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $click ORDER BY ElseGroup,ConditionTypeOrReference,ConditionTarget;
SELECT 'root-row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $root ORDER BY ElseGroup,ConditionTypeOrReference,ConditionTarget;
SELECT 'outgoing-row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $outgoing ORDER BY ElseGroup,ConditionTypeOrReference,ConditionTarget;
"@
$reset=@"
DELETE FROM conditions WHERE ($click) OR ($root) OR ($outgoing);
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2) VALUES(18,106854,212060,0,0,9,0,41574,0),(18,106854,212060,0,1,28,0,41574,0),(22,3,106854,0,0,44,1,1099,5);
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,NegativeCondition) VALUES(22,1,106854,0,0,9,0,41574,0,0),(22,1,106854,0,0,44,1,1099,5,1),(22,1,106854,0,1,28,0,41574,0,0),(22,1,106854,0,1,44,1,1099,5,1),(22,1,106854,0,2,8,0,41574,0,0),(22,1,106854,0,2,44,1,1099,5,1);
UPDATE creature_template current JOIN saved_actor old USING(entry) SET current.ScriptName=old.ScriptName;
UPDATE npc_spellclick_spells current JOIN saved_click old ON current.npc_entry=old.npc_entry AND current.spell_id=old.spell_id SET current.cast_flags=old.cast_flags;
DELETE FROM smart_scripts;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
"@
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=106854;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
INSERT INTO creature_template SELECT * FROM saved_actor;
CREATE TEMPORARY TABLE saved_click LIKE npc_spellclick_spells;
INSERT INTO saved_click SELECT * FROM npc_spellclick_spells WHERE npc_entry=106854;
CREATE TEMPORARY TABLE npc_spellclick_spells LIKE saved_click;
INSERT INTO npc_spellclick_spells SELECT * FROM saved_click;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=106854 AND source_type=0;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE (SourceTypeOrReferenceId=18 AND SourceGroup=106854) OR (SourceTypeOrReferenceId=22 AND SourceEntry=106854 AND SourceId=0);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
$reset
SELECT 'baseline';
$snapshot
SELECT 'before-smart',current.* FROM smart_scripts current ORDER BY id;
$migration
$migration
SELECT 'after-smart',current.* FROM smart_scripts current ORDER BY id;
SELECT 'fixed';
$snapshot
SELECT 'click-count',COUNT(*) FROM conditions WHERE $click;
SELECT 'root-count',COUNT(*) FROM conditions WHERE $root;
SELECT 'outgoing-count',COUNT(*) FROM conditions WHERE $outgoing;
SELECT 'outgoing-native',COUNT(*) FROM conditions WHERE $outgoing AND ConditionTypeOrReference IN(8,9,28,44);
SELECT 'outgoing',COUNT(*) FROM conditions WHERE $click AND ((ElseGroup=0 AND ConditionTypeOrReference=9) OR (ElseGroup=1 AND ConditionTypeOrReference=28)) AND ConditionTarget=0 AND ConditionValue1=41574 AND NegativeCondition=0;
SELECT 'actor',COUNT(*) FROM creature_template current JOIN saved_actor old USING(entry) WHERE current.AIName=old.AIName AND current.ScriptName=old.ScriptName AND current.npcflag=old.npcflag;
SELECT 'click-metadata',COUNT(*) FROM npc_spellclick_spells current JOIN saved_click old USING(npc_entry,spell_id) WHERE current.cast_flags=old.cast_flags AND current.user_type=old.user_type AND current.add_npc_flag=old.add_npc_flag;
SELECT 'other',COUNT(*) FROM conditions current JOIN saved_conditions old ON current.SourceTypeOrReferenceId=old.SourceTypeOrReferenceId AND current.SourceGroup=old.SourceGroup AND current.SourceEntry=old.SourceEntry AND current.SourceId=old.SourceId AND current.ElseGroup=old.ElseGroup AND current.ConditionTypeOrReference=old.ConditionTypeOrReference AND current.ConditionTarget=old.ConditionTarget AND current.ConditionValue1=old.ConditionValue1 AND current.ConditionValue2=old.ConditionValue2 AND current.ConditionValue3=old.ConditionValue3 AND current.NegativeCondition=old.NegativeCondition WHERE current.SourceTypeOrReferenceId=22 AND current.SourceGroup NOT IN(1,3);
SELECT 'saved-other',COUNT(*) FROM saved_conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup NOT IN(1,3);
"@
$variants=[ordered]@{
 'custom-script'="UPDATE creature_template SET ScriptName='custom_huey';"
 'custom-cast'='UPDATE npc_spellclick_spells SET cast_flags=3;'
 'custom-event'='UPDATE smart_scripts SET event_type=64 WHERE id=2;'
 'custom-credit'='UPDATE smart_scripts SET action_param2=123 WHERE id=2;'
 'custom-target'='UPDATE smart_scripts SET target_type=21 WHERE id=2;'
 'custom-teleport'='UPDATE smart_scripts SET action_param1=1495 WHERE id=3;'
 'custom-extra'='INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,event_chance,action_type,target_type,comment) VALUES(106854,0,20,64,100,1,1,''Custom added event'');'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`n$migration`nSELECT '$name',COUNT(*) FROM conditions WHERE (($click) AND ElseGroup>1) OR (($root) AND ConditionTarget=0);`n"}
$sql+=@"
$reset
UPDATE conditions SET ConditionValue1=12345 WHERE $click;
$migration
SELECT 'custom-click',COUNT(*) FROM conditions WHERE $click;
SELECT 'custom-click-kept',COUNT(*) FROM conditions WHERE $click AND ConditionValue1=12345;
$reset
UPDATE conditions SET ConditionTypeOrReference=4,ConditionValue1=12345,ConditionValue2=0 WHERE $root;
$migration
SELECT 'custom-root',COUNT(*) FROM conditions WHERE $root;
SELECT 'custom-root-kept',COUNT(*) FROM conditions WHERE $root AND ConditionTypeOrReference=4 AND ConditionValue1=12345;
$reset
UPDATE conditions SET ConditionValue1=12345 WHERE $outgoing AND ConditionTypeOrReference=9;
$migration
SELECT 'custom-outgoing',COUNT(*) FROM conditions WHERE $outgoing;
SELECT 'custom-outgoing-kept',COUNT(*) FROM conditions WHERE $outgoing AND ConditionTypeOrReference=9 AND ConditionValue1=12345;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Huey TEMP fixture failed'}}finally{$env:MYSQL_PWD=$previous}
[IO.File]::WriteAllLines("$out/fixture-diagnostics.tsv",[string[]]$rows)
foreach($expected in @("click-count`t20","root-count`t16","outgoing-count`t9","outgoing-native`t6","outgoing`t2","actor`t1","click-metadata`t1","custom-click`t2","custom-click-kept`t2","custom-root`t1","custom-root-kept`t1","custom-outgoing`t6","custom-outgoing-kept`t1")+@($variants.Keys|ForEach-Object {"$_`t0"})){if($rows -notcontains $expected){throw "Huey mismatch: $expected"}}
if((($rows|Where-Object {$_ -like "other`t*"}) -replace '^other','') -ne (($rows|Where-Object {$_ -like "saved-other`t*"}) -replace '^saved-other','')){throw 'Outgoing SmartAI conditions changed'}
$before=@($rows|Where-Object {$_ -like "before-smart`t*"}|ForEach-Object {$_.Substring(13)});$after=@($rows|Where-Object {$_ -like "after-smart`t*"}|ForEach-Object {$_.Substring(12)})
if(Compare-Object $before $after){throw 'Native Huey actions changed'}
$baseline=[Array]::IndexOf($rows,'baseline');$fixed=[Array]::IndexOf($rows,'fixed')
foreach($mode in @('baseline','fixed')){
 $selected=if($mode -eq 'baseline'){$rows[($baseline+1)..($fixed-1)]}else{$rows[($fixed+1)..($rows.Count-1)]}
 foreach($kind in @('click','root','outgoing')){[IO.File]::WriteAllLines("$out/$mode-$kind.tsv",[string[]]@($selected|Where-Object {$_ -like "$kind-row`t*"}|ForEach-Object {$_.Substring($kind.Length+5)}))}
}
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$cases=(Section $source '        case CONDITION_ITEM:' '        case CONDITION_ITEM_EQUIPPED:')+(Section $source '        case CONDITION_CLASS:' '        case CONDITION_RACE:')+(Section $source '        case CONDITION_QUESTREWARDED:' '        case CONDITION_QUEST_NONE:')+(Section $source '        case CONDITION_MAPID:' '        case CONDITION_AREAID:')+(Section $source '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:')+(Section $source '        case CONDITION_ALIVE:' '        case CONDITION_HP_VAL:')
[IO.File]::WriteAllText("$out/Conditions.inc",$cases)
[IO.File]::WriteAllText("$out/ElseGroups.inc",(Section $source 'bool ConditionMgr::IsObjectMeetToConditionList(' 'bool ConditionMgr::IsObjectMeetToConditions(WorldObject* object,'))
[IO.File]::WriteAllText("$out/ClickConditions.inc",(Section $source 'ConditionList ConditionMgr::GetConditionsForSpellClickEvent(' 'ConditionList ConditionMgr::GetConditionsForVehicleSpell('))
[IO.File]::WriteAllText("$out/SmartEventConditions.inc",(Section $source 'bool ConditionMgr::IsObjectMeetingSmartEventConditions(' 'bool Condition::isLoaded() const'))
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
[IO.File]::WriteAllText("$out/ClickVisibility.inc",(Section $source 'bool Player::canSeeSpellClickOn(' 'void Player::SendTalentsInfoData('))
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
[IO.File]::WriteAllText("$out/HandleSpellClick.inc",(Section $source 'bool Unit::HandleSpellClick(' 'void Unit::EnterVehicle('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/HueyReturnAdmissionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/baseline-click.tsv`" `"$out/baseline-root.tsv`" `"$out/baseline-outgoing.tsv`" baseline`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/fixed-click.tsv`" `"$out/fixed-root.tsv`" `"$out/fixed-outgoing.tsv`" fixed`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Huey click/return admission failed'}
if($VerifyReleaseBinary){
 $previousPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_huey_return_admission_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Deployed Huey click/condition bridge failed'}}finally{$env:PYTHONPATH=$previousPython}
}
'PASS: atomic repeat Huey migration restores only final claimed-owner click eligibility, preserves initial41574 branches/native actions/custom scopes, and gates actor-driven return even when a rejected spellclick still calls AI'
