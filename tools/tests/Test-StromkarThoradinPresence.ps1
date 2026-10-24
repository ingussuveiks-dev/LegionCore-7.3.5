param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-thoradin-presence"
New-Item -ItemType Directory -Force $out|Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_51_stromkar_thoradin_presence.sql" -Raw
$gate='SourceTypeOrReferenceId=22 AND SourceGroup=10 AND SourceEntry=104276 AND SourceId=0'
$cast='SourceTypeOrReferenceId=17 AND SourceEntry=206456'
$near="$cast AND ConditionTypeOrReference=29 AND ConditionValue1=104307 AND ConditionValue2=50"
$tables=@('smart_scripts','conditions','creature_template','creature_template_wdb','waypoints','creature','creature_summon_groups','spell_linked_spell')
$reset=($tables|ForEach-Object {"DELETE FROM $_;INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$reset+="`nDELETE FROM smart_scripts WHERE entryorguid=104276 AND source_type=0 AND id=9;DELETE FROM conditions WHERE ($gate) OR ($near);"
$sql=@'
CREATE TEMPORARY TABLE saved_smart_scripts LIKE smart_scripts;INSERT INTO saved_smart_scripts SELECT * FROM smart_scripts WHERE entryorguid IN(104276,104307,103151);
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;INSERT INTO saved_conditions SELECT * FROM conditions WHERE (SourceTypeOrReferenceId=17 AND SourceEntry=206456) OR (SourceTypeOrReferenceId=22 AND SourceEntry=104276);
CREATE TEMPORARY TABLE saved_creature_template LIKE creature_template;INSERT INTO saved_creature_template SELECT * FROM creature_template WHERE entry IN(104276,104307);
CREATE TEMPORARY TABLE saved_creature_template_wdb LIKE creature_template_wdb;INSERT INTO saved_creature_template_wdb SELECT * FROM creature_template_wdb WHERE Entry=104307;
CREATE TEMPORARY TABLE saved_waypoints LIKE waypoints;INSERT INTO saved_waypoints SELECT * FROM waypoints WHERE entry=104307;
CREATE TEMPORARY TABLE saved_creature LIKE creature;INSERT INTO saved_creature SELECT * FROM creature WHERE id=104307;
CREATE TEMPORARY TABLE saved_creature_summon_groups LIKE creature_summon_groups;INSERT INTO saved_creature_summon_groups SELECT * FROM creature_summon_groups WHERE entry=104307;
CREATE TEMPORARY TABLE saved_spell_linked_spell LIKE spell_linked_spell;INSERT INTO saved_spell_linked_spell SELECT * FROM spell_linked_spell WHERE spell_trigger=206456;
CREATE TEMPORARY TABLE sample_creature LIKE creature;INSERT INTO sample_creature SELECT * FROM creature WHERE id=103144 LIMIT 1;
CREATE TEMPORARY TABLE sample_group LIKE creature_summon_groups;INSERT INTO sample_group SELECT * FROM creature_summon_groups LIMIT 1;
'@
foreach($table in $tables){$sql+="`nCREATE TEMPORARY TABLE $table LIKE saved_$table;"}
$sql+="$reset`nSELECT 'old-before',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;SELECT 'conditions-before',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,ElseGroup,ConditionTypeOrReference;`n$migration`nSELECT 'first',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;SELECT 'first-gates',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,ElseGroup,ConditionTypeOrReference;`n$migration`nSELECT 'second',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;SELECT 'second-gates',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,ElseGroup,ConditionTypeOrReference;`nSELECT 'old-after',smart_scripts.* FROM smart_scripts WHERE NOT(entryorguid=104276 AND source_type=0 AND id=9) ORDER BY entryorguid,source_type,id;SELECT 'conditions-after',conditions.* FROM conditions WHERE NOT(($gate) OR ($near)) ORDER BY SourceTypeOrReferenceId,SourceGroup,ElseGroup,ConditionTypeOrReference;`nSELECT 'row',entryorguid,source_type,id,link,event_type,event_phase_mask,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,action_param3,target_type,target_x,target_y,target_z,target_o FROM smart_scripts WHERE entryorguid=104276 AND source_type=0 AND id=9;SELECT 'gate',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $gate ORDER BY ElseGroup,ConditionTypeOrReference;SELECT 'cast',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $cast ORDER BY ElseGroup,ConditionTypeOrReference;`n"
$variants=[ordered]@{
 'custom-boss'="UPDATE creature_template SET ScriptName='custom' WHERE entry=104276;"
 'custom-actor'="UPDATE creature_template SET ScriptName='custom' WHERE entry=104307;"
 'custom-flags'='UPDATE creature_template SET npcflag=1 WHERE entry=104307;'
 'custom-model'='UPDATE creature_template_wdb SET Displayid1=1;'
 'custom-position'='UPDATE waypoints SET position_z=1;'
 'custom-path'='INSERT INTO waypoints(entry,pointid,position_x,position_y,position_z) VALUES(104307,2,1,1,1);'
 'existing-spawn'='INSERT INTO creature SELECT * FROM sample_creature;UPDATE creature SET id=104307;'
 'existing-group'='INSERT INTO creature_summon_groups SELECT * FROM sample_group;UPDATE creature_summon_groups SET entry=104307;'
 'custom-defeat'='UPDATE smart_scripts SET event_param2=2 WHERE entryorguid=104276 AND source_type=0 AND id=3;'
 'custom-hit'='UPDATE smart_scripts SET event_param1=206456 WHERE entryorguid=104276 AND source_type=0 AND id=8;'
 'custom-departure'='UPDATE smart_scripts SET action_param1=2 WHERE entryorguid=104276 AND source_type=9 AND id=1;'
 'custom-intro'='UPDATE smart_scripts SET action_param1=3 WHERE entryorguid=104307 AND source_type=0 AND id=0;'
 'custom-receiver'='UPDATE smart_scripts SET event_param2=2 WHERE entryorguid=104307 AND source_type=0 AND id=3;'
 'occupied-id'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,comment) VALUES(104276,0,9,60,1,'custom');"
 'occupied-gate'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ConditionTypeOrReference,ConditionValue1) VALUES(22,10,104276,0,22,1539);"
 'custom-cast-stage'="UPDATE conditions SET ConditionValue2=8 WHERE $cast AND ConditionTypeOrReference=44;"
 'custom-cast-notice'="UPDATE conditions SET ErrorTextId=123 WHERE $cast AND ConditionTypeOrReference=3;"
 'custom-linked'='UPDATE spell_linked_spell SET spell_effect=206455;'
 'custom-linked-chance'='UPDATE spell_linked_spell SET chance=50;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;SELECT '$name-gates-before',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,ElseGroup,ConditionTypeOrReference;`n$migration`nSELECT '$name-after',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;SELECT '$name-gates-after',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,ElseGroup,ConditionTypeOrReference;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Thoradin TEMP query failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/fixture-diagnostics.tsv",[string[]]$rows)
function Snapshot([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
function Signature([string]$label){[string]::Join("`n",[string[]]@(Snapshot $label))}
foreach($pair in @(@('first','second'),@('first-gates','second-gates'),@('old-before','old-after'),@('conditions-before','conditions-after'))){if((Signature $pair[0]) -cne (Signature $pair[1])){throw "Snapshot changed: $pair"}}
foreach($name in $variants.Keys){foreach($suffix in @('','-gates')){if((Signature "$name$suffix-before") -cne (Signature "$name$suffix-after")){throw "Custom content overwritten: $name"}}}
foreach($kind in @('row','gate','cast')){[IO.File]::WriteAllLines("$out/$kind.tsv",[string[]](Snapshot $kind))}
if(@(Snapshot 'row').Count -ne 1 -or @(Snapshot 'gate').Count -ne 6 -or @(Snapshot 'cast').Count -ne 18){throw 'Expected one retry/six base-actor rules/eighteen cast rules'}
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$cases=(Section $source '        case CONDITION_MAPID:' '        case CONDITION_AREAID:')+(Section $source '        case CONDITION_ALIVE:' '        case CONDITION_HP_VAL:')+(Section $source '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:')
$cases+=(Section $source '        case CONDITION_AURA:' '        case CONDITION_ITEM:')+(Section $source '        case CONDITION_ITEM_EQUIPPED:' '        case CONDITION_ZONEID:')+(Section $source '        case CONDITION_CLASS:' '        case CONDITION_RACE:')+(Section $source '        case CONDITION_QUESTREWARDED:' '        case CONDITION_QUEST_NONE:')
$cases+=(Section $source '        case CONDITION_NEAR_CREATURE:' '        case CONDITION_NEAR_GAMEOBJECT:')+(Section $source '        case CONDITION_OBJECT_ENTRY:' '        case CONDITION_TYPE_MASK:')+(Section $source '        case CONDITION_HP_PCT:' '        case CONDITION_WORLD_STATE:')
[IO.File]::WriteAllText("$out/Conditions.inc",$cases)
[IO.File]::WriteAllText("$out/ElseGroups.inc",(Section $source 'bool ConditionMgr::IsObjectMeetToConditionList(' 'bool ConditionMgr::IsObjectMeetToConditions(WorldObject* object,'))
$first=$source.IndexOf("template <class T>`r`nbool CompareValues");if($first -lt 0){$first=$source.IndexOf("template <class T>`nbool CompareValues")};if($first -lt 0){throw 'Missing comparisons'}
[IO.File]::WriteAllText("$out/Compare.inc",$source.Substring($first))
$source=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
[IO.File]::WriteAllText("$out/CastConditions.inc",(Section $source '    ConditionSourceInfo condInfo = ConditionSourceInfo(m_caster);' "    // Don't check explicit target for passive spells"))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$action=Section $source '        case SMART_ACTION_SUMMON_CREATURE:' '        case SMART_ACTION_SUMMON_GO:'
$first=$action.LastIndexOf('            if (e.GetTargetType() == SMART_TARGET_POSITION)');$last=$action.LastIndexOf('            break;')
if($first -lt 0 -or $last -le $first){throw 'Missing native fixed creature branch'}
[IO.File]::WriteAllText("$out/FixedCreatureSummon.inc",$action.Substring($first,$last-$first))
[IO.File]::WriteAllText("$out/RecalcTimer.inc",(Section $source 'void SmartScript::RecalcTimer(' 'void SmartScript::UpdateTimer('))
[IO.File]::WriteAllText("$out/UpdateEvent.inc",(Section $source 'void SmartScript::ProcessEvent(' '        case SMART_EVENT_UPDATE_OOC:')+"        default: assert(false);`n    }`n}`n")
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/StromkarThoradinPresenceTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/row.tsv`" `"$out/gate.tsv`" `"$out/cast.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Thoradin presence failed'}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile StromkarPickupCatalog.cpp -ProbeName StromkarPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$?){throw 'Native Thoradin model failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv" -Raw
foreach($expected in @("THORADIN_MODEL`t68038`t49`t119940`t1.4`t255","THORADIN_VISUAL`t136052`t59520`t2","THORADIN_KIT`t59520`t0`t0`t0","THORADIN_KIT_EFFECT`t123138`t2`t310837","THORADIN_KIT_EFFECT`t123139`t1`t8357","THORADIN_ATTACHMENT`t310837`t13651`t19`t1`t0`t166237`t0.4`t0","THORADIN_PROCEDURE`t8357`t22`t0`t0`t0.4")){if(!$native.Contains($expected)){throw "Native Thoradin visual chain differs: $expected"}}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_51_stromkar_thoradin_presence.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_51_stromkar_thoradin_presence.sql';SELECT 'row',entryorguid,source_type,id,link,event_type,event_phase_mask,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,action_param3,target_type,target_x,target_y,target_z,target_o FROM smart_scripts WHERE entryorguid=104276 AND source_type=0 AND id=9;SELECT 'gate',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $gate ORDER BY ElseGroup,ConditionTypeOrReference;SELECT 'cast',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $cast ORDER BY ElseGroup,ConditionTypeOrReference;"
 if($live -notcontains "release`t$hash`tRELEASED"){throw 'Live51 hash differs'}
 foreach($kind in @('row','gate','cast')){$actual=@($live|Where-Object {$_ -like "$kind`t*"}|ForEach-Object {$_.Substring($kind.Length+1)});if(Compare-Object (Snapshot $kind) $actual){throw "Live $kind differs from fixture"}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_stromkar_thoradin_presence_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Thoradin consumers failed'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: final Thoradin104307 is retried only after actual final-stage defeat, using existing waypoint/model/Smart dialogue; Execute waits for a live phase-near receiver. Native/SQL/model checks are not actual actor visibility, farewell, cleanup or client traversal proof.'
