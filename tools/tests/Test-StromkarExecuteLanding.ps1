param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-execute-landing"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_50_stromkar_execute_landing.sql" -Raw
$scope='SourceTypeOrReferenceId=17 AND SourceEntry=206456'
$reset=@"
DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actor;
DELETE FROM gameobject_template; INSERT INTO gameobject_template SELECT * FROM saved_object;
DELETE FROM conditions; INSERT INTO conditions SELECT * FROM saved_conditions;
DELETE FROM conditions WHERE $scope;
DELETE FROM spell_target_position; INSERT INTO spell_target_position SELECT * FROM saved_position;
DELETE FROM spell_script_names; INSERT INTO spell_script_names SELECT * FROM saved_scripts;
"@
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE creature_template; INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=104276;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
CREATE TEMPORARY TABLE saved_object LIKE gameobject_template; INSERT INTO saved_object SELECT * FROM gameobject_template WHERE entry=247877;
CREATE TEMPORARY TABLE gameobject_template LIKE saved_object;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions; INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceEntry IN(206455,206456,201904);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
CREATE TEMPORARY TABLE saved_position LIKE spell_target_position; INSERT INTO saved_position SELECT * FROM spell_target_position WHERE id=206456;
CREATE TEMPORARY TABLE spell_target_position LIKE saved_position;
CREATE TEMPORARY TABLE saved_scripts LIKE spell_script_names; INSERT INTO saved_scripts SELECT * FROM spell_script_names WHERE spell_id=206456;
CREATE TEMPORARY TABLE spell_script_names LIKE saved_scripts;
$reset
SELECT 'actor-before',creature_template.* FROM creature_template;
SELECT 'object-before',gameobject_template.* FROM gameobject_template;
SELECT 'other-before',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;
$migration
SELECT 'first',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;
$migration
SELECT 'second',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;
SELECT 'actor-after',creature_template.* FROM creature_template;
SELECT 'object-after',gameobject_template.* FROM gameobject_template;
SELECT 'other-after',conditions.* FROM conditions WHERE NOT($scope) ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;
SELECT 'row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY ElseGroup,ConditionTypeOrReference;
"@
$variants=[ordered]@{
 'custom-actor-ai'="UPDATE creature_template SET AIName='';"
 'custom-actor-script'="UPDATE creature_template SET ScriptName='custom';"
 'custom-object-type'='UPDATE gameobject_template SET type=10;'
 'custom-object-loot'='UPDATE gameobject_template SET Data1=0;'
 'custom-object-ai'="UPDATE gameobject_template SET AIName='';"
 'custom-object-script'="UPDATE gameobject_template SET ScriptName='custom';"
 'missing-launch'='DELETE FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=206455;'
 'custom-launch-stage'='UPDATE conditions SET ConditionValue2=8 WHERE SourceTypeOrReferenceId=13 AND SourceEntry=206455 AND ConditionTypeOrReference=44;'
 'custom-launch-script'="UPDATE conditions SET ScriptName='custom' WHERE SourceTypeOrReferenceId=13 AND SourceEntry=206455 AND ConditionTypeOrReference=3;"
 'custom-launch-target'='UPDATE conditions SET ConditionTarget=0 WHERE SourceTypeOrReferenceId=13 AND SourceEntry=206455 AND ConditionTypeOrReference=3;'
 'custom-launch-notice'='UPDATE conditions SET ErrorTextId=123 WHERE SourceTypeOrReferenceId=13 AND SourceEntry=206455 AND ConditionTypeOrReference=3;'
 'custom-launch-group'='UPDATE conditions SET SourceGroup=2 WHERE SourceTypeOrReferenceId=13 AND SourceEntry=206455 AND ConditionTypeOrReference=3;'
 'custom-cast'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(17,206456,22,1);"
 'custom-arrival-target'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1,ConditionValue2) VALUES(13,1,206456,31,3,1);"
 'custom-script'="INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(206456,'custom');"
 'custom-position'='INSERT INTO spell_target_position(id,target_map,target_position_x,target_position_y,target_position_z,target_orientation) VALUES(206456,1539,1,1,1,0);'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;`n$migration`nSELECT '$name-after',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Landing TEMP query failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/fixture-diagnostics.tsv",[string[]]$rows)
function Snapshot([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
function Signature([string]$label){[string]::Join("`n",[string[]]@(Snapshot $label))}
foreach($pair in @(@('first','second'),@('actor-before','actor-after'),@('object-before','object-after'),@('other-before','other-after'))){if((Signature $pair[0]) -cne (Signature $pair[1])){throw "Changed $pair"}}
foreach($name in $variants.Keys){if((Signature "$name-before") -cne (Signature "$name-after")){throw "Custom landing changed: $name"}}
$nativeRows=Snapshot 'row';if($nativeRows.Count -ne 16){throw 'Expected16 landing rules'}
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]]$nativeRows)
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$cases=(Section $source '        case CONDITION_MAPID:' '        case CONDITION_AREAID:')+(Section $source '        case CONDITION_ALIVE:' '        case CONDITION_HP_VAL:')+(Section $source '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:')
$cases+=(Section $source '        case CONDITION_AURA:' '        case CONDITION_ITEM:')+(Section $source '        case CONDITION_ITEM_EQUIPPED:' '        case CONDITION_ZONEID:')+(Section $source '        case CONDITION_CLASS:' '        case CONDITION_RACE:')+(Section $source '        case CONDITION_QUESTREWARDED:' '        case CONDITION_QUEST_NONE:')
[IO.File]::WriteAllText("$out/Conditions.inc",$cases)
[IO.File]::WriteAllText("$out/ElseGroups.inc",(Section $source 'bool ConditionMgr::IsObjectMeetToConditionList(' 'bool ConditionMgr::IsObjectMeetToConditions(WorldObject* object,'))
[IO.File]::WriteAllText("$out/NotGroupedConditions.inc",(Section $source 'ConditionList ConditionMgr::GetConditionsForNotGroupedEntry(' 'ConditionList ConditionMgr::GetConditionsForSpellClickEvent('))
$source=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
[IO.File]::WriteAllText("$out/CastConditions.inc",(Section $source '    ConditionSourceInfo condInfo = ConditionSourceInfo(m_caster);' "    // Don't check explicit target for passive spells"))
$source=Get-Content "$repo/src/server/game/Movement/MovementGenerators/PointMovementGenerator.cpp" -Raw
$first=$source.IndexOf('void EffectMovementGenerator::Finalize(');if($first -lt 0){throw 'Missing native arrival'}
[IO.File]::WriteAllText("$out/Arrival.inc",$source.Substring($first))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/StromkarExecuteLandingTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/rows.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Execute landing failed'}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile StromkarPickupCatalog.cpp -ProbeName StromkarPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$?){throw 'Native landing catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv" -Raw
foreach($expected in @("EFFECT`t206455`t0`t42`t0`t35`t206456`t46`t0","EFFECT`t206456`t0`t61`t0`t49109","EFFECT`t206456`t1`t203`t0`t0`t206448","EFFECT`t206456`t2`t90`t0`t103114","ARRIVAL_REFERENCE`t206455`t0`t0")){if(!$native.Contains($expected)){throw "Native arrival chain changed: $expected"}}
if(@(Get-Content "$out/native.tsv"|Where-Object {$_ -match '^ARRIVAL_REFERENCE\t'}).Count -ne 1){throw 'Additional native arrival use requires review'}
$usage=Query "SELECT 'smart-arrival',COUNT(*) FROM smart_scripts WHERE action_type IN(11,85) AND action_param1=206456; SELECT 'click-arrival',COUNT(*) FROM npc_spellclick_spells WHERE spell_id=206456;"
if($usage -notcontains "smart-arrival`t0" -or $usage -notcontains "click-arrival`t0"){throw 'Additional live arrival usage requires review'}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_50_stromkar_execute_landing.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_50_stromkar_execute_landing.sql'; SELECT 'row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY ElseGroup,ConditionTypeOrReference;"
 if($live -notcontains "release`t$hash`tRELEASED"){throw 'Live50 hash mismatch'}
 $actual=@($live|Where-Object {$_ -like "row`t*"}|ForEach-Object {$_.Substring(4)})
 if($actual.Count -eq 18){
  $followupHash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_51_stromkar_thoradin_presence.sql" -Algorithm SHA1).Hash
  $followup=Query "SELECT 'followup',hash,state FROM updates WHERE name='2026_10_24_51_stromkar_thoradin_presence.sql';SELECT 'extras',COUNT(*) FROM conditions WHERE $scope AND SourceGroup=0 AND SourceId=0 AND ConditionTypeOrReference=29 AND ConditionTarget=0 AND ConditionValue1=104307 AND ConditionValue2=50 AND ConditionValue3=0 AND NegativeCondition=0 AND ErrorTextId=0 AND ScriptName='' AND ElseGroup IN(0,1);"
  if($followup -notcontains "followup`t$followupHash`tRELEASED" -or $followup -notcontains "extras`t2"){throw 'Unrecognized later landing rules'}
  $extraRows=@("0`t29`t0`t104307`t50`t0`t0","1`t29`t0`t104307`t50`t0`t0")
  foreach($extra in $extraRows){if($actual -notcontains $extra){throw 'Missing known51 actor rule'}}
  $actual=@($actual|Where-Object {$_ -notin $extraRows})
 }
 if(Compare-Object $nativeRows $actual){throw 'Live landing differs from tested rows'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_stromkar_execute_landing_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical landing consumer failed'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: native206456 jump arrival rechecks active living equipped Warrior41105/1539/1037/exact9/native bar before unchanged events;16 guarded SQL rules/native predicates/arrival/reverse references checked. Actual interrupted movement, actor/departure, credit and client VFX remain separate.'
