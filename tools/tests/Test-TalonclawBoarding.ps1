param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/talonclaw-boarding"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_26_talonclaw_boarding_admission.sql" -Raw
$scopes='(SourceTypeOrReferenceId=18 AND SourceGroup=100965 AND SourceEntry=198941) OR (SourceTypeOrReferenceId=17 AND SourceEntry=198941) OR (SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=100965 AND SourceId=0)'
$reset=@"
DELETE FROM conditions;
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1) VALUES(22,1,100965,0,0,9,0,40385);
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actor;
DELETE FROM npc_spellclick_spells; INSERT INTO npc_spellclick_spells SELECT * FROM saved_click;
"@
$snapshot="SELECT 'row',SourceTypeOrReferenceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions ORDER BY SourceTypeOrReferenceId,ElseGroup,ConditionTypeOrReference,ConditionTarget;"
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=100965;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=100965 AND source_type=0;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
CREATE TEMPORARY TABLE saved_click LIKE npc_spellclick_spells;
INSERT INTO saved_click SELECT * FROM npc_spellclick_spells WHERE npc_entry=100965;
CREATE TEMPORARY TABLE npc_spellclick_spells LIKE saved_click;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
$reset
SELECT 'baseline';
$snapshot
SELECT 'BEFORE',smart_scripts.* FROM smart_scripts ORDER BY id;
$migration
SELECT 'FIRST',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,ConditionTypeOrReference,ConditionTarget;
$migration
SELECT 'SECOND',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,ConditionTypeOrReference,ConditionTarget;
SELECT 'AFTER',smart_scripts.* FROM smart_scripts ORDER BY id;
SELECT 'count',SourceTypeOrReferenceId,COUNT(*) FROM conditions GROUP BY SourceTypeOrReferenceId;
SELECT 'fixed';
$snapshot
"@
$variants=[ordered]@{
 'custom-ai'="UPDATE creature_template SET AIName='';"
 'custom-script'="UPDATE creature_template SET ScriptName='custom_eagle';"
 'custom-click'='UPDATE npc_spellclick_spells SET cast_flags=2;'
 'custom-credit'='UPDATE smart_scripts SET action_param1=1 WHERE id=0;'
 'custom-travel'='UPDATE smart_scripts SET action_param1=1 WHERE id=1;'
 'custom-root'='UPDATE conditions SET ConditionValue1=1;'
 'custom-click-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(18,100965,198941,9,1);"
 'custom-cast-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(17,0,198941,9,1);"
 'custom-link'='UPDATE smart_scripts SET link=0 WHERE id=0;'
 'custom-chance'='UPDATE smart_scripts SET event_chance=50 WHERE id=0;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`n$migration`nSELECT '$name',COUNT(*) FROM conditions WHERE Comment='Talonclaw: admit only the living Dalaran Hunter claimant and boarding hippogryph';`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$rows=@($query | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Boarding SQL regression failed'};return $rows}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
foreach($expected in @("count`t17`t5","count`t18`t8","count`t22`t8")){if($rows -notcontains $expected){throw "Boarding mismatch: $expected"}}
foreach($name in $variants.Keys){if($rows -notcontains "$name`t0"){throw "Custom boarding scope overwritten: $name"}}
$first=@($rows|Where-Object {$_ -like "FIRST`t*"}|ForEach-Object {$_.Substring(6)});$second=@($rows|Where-Object {$_ -like "SECOND`t*"}|ForEach-Object {$_.Substring(7)})
if(Compare-Object $first $second){throw 'Boarding migration not idempotent'}
$before=@($rows|Where-Object {$_ -like "BEFORE`t*"}|ForEach-Object {$_.Substring(7)});$after=@($rows|Where-Object {$_ -like "AFTER`t*"}|ForEach-Object {$_.Substring(6)})
if(Compare-Object $before $after){throw 'Boarding native credit/travel actions changed'}
$start=[Array]::IndexOf($rows,'baseline');$fixed=[Array]::IndexOf($rows,'fixed')
$selected=if($Baseline){$rows[($start+1)..($fixed-1)]}else{$rows[($fixed+1)..($rows.Count-1)]}
foreach($scope in @(@('click',18),@('cast',17),@('root',22))){[IO.File]::WriteAllLines("$out/$($scope[0]).tsv",[string[]]@($selected | Where-Object {$_ -like "row`t$($scope[1])`t*"} | ForEach-Object {($_.Split("`t") | Select-Object -Skip 2) -join "`t"}))}
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$cases=(Section $source '        case CONDITION_ITEM:' '        case CONDITION_ITEM_EQUIPPED:')+(Section $source '        case CONDITION_CLASS:' '        case CONDITION_RACE:')+(Section $source '        case CONDITION_QUESTREWARDED:' '        case CONDITION_QUEST_NONE:')+(Section $source '        case CONDITION_MAPID:' '        case CONDITION_AREAID:')+(Section $source '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:')+(Section $source '        case CONDITION_ALIVE:' '        case CONDITION_HP_VAL:')+(Section $source '        case CONDITION_OBJECT_ENTRY:' '        case CONDITION_TYPE_MASK:')
[IO.File]::WriteAllText("$out/Conditions.inc",$cases)
[IO.File]::WriteAllText("$out/ElseGroups.inc",(Section $source 'bool ConditionMgr::IsObjectMeetToConditionList(' 'bool ConditionMgr::IsObjectMeetToConditions(WorldObject* object,'))
[IO.File]::WriteAllText("$out/ClickConditions.inc",(Section $source 'ConditionList ConditionMgr::GetConditionsForSpellClickEvent(' 'ConditionList ConditionMgr::GetConditionsForVehicleSpell('))
[IO.File]::WriteAllText("$out/SmartEventConditions.inc",(Section $source 'bool ConditionMgr::IsObjectMeetingSmartEventConditions(' 'bool Condition::isLoaded() const'))
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
[IO.File]::WriteAllText("$out/ClickVisibility.inc",(Section $source 'bool Player::canSeeSpellClickOn(' 'void Player::SendTalentsInfoData('))
function Removal([string]$source){$method=Section $source 'void Player::RemoveActiveQuest(' 'void Player::RemoveRewardedQuest('; $begin=$method.IndexOf('        PhaseUpdateData phaseUdateData;');$end=$method.IndexOf('    });',$begin);if($begin -lt 0 -or $end -le $begin){throw 'Missing removal callback'};$method.Substring($begin,$end-$begin)}
[IO.File]::WriteAllText("$out/Removal.inc",(Removal $source))
$old=(& git -C $repo show '180f4ca3:src/server/game/Entities/Player/Player.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned removal source unavailable'}
[IO.File]::WriteAllText("$out/OldRemoval.inc",(Removal $old))
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
[IO.File]::WriteAllText("$out/HandleSpellClick.inc",(Section $source 'bool Unit::HandleSpellClick(' 'void Unit::EnterVehicle('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'baseline'}else{'fixed'}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" /I`"$PSScriptRoot`" `"$PSScriptRoot/TalonclawBoardingTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/click.tsv`" `"$out/root.tsv`" `"$out/cast.tsv`" $mode`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native boarding admission failed'}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TalonclawBoardingCatalog.cpp -ProbeName TalonclawBoardingCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv"
foreach($expected in @("EFFECT`t198941`t0`t28`t0`t100967`t0`t46`t0`t61","EFFECT`t198950`t0`t227`t0`t1321`t0`t1`t17`t1384")){if($native -notcontains $expected){throw "Native boarding effect changed: $expected"}}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_26_talonclaw_boarding_admission.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_26_talonclaw_boarding_admission.sql'; SELECT 'count',SourceTypeOrReferenceId,COUNT(*) FROM conditions WHERE $scopes GROUP BY SourceTypeOrReferenceId;"
 foreach($expected in @("release`t$hash`tRELEASED","count`t17`t5","count`t18`t8","count`t22`t8")){if($live -notcontains $expected){throw "Boarding deployment mismatch: $expected"}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_talonclaw_boarding_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical boarding path missing'}}finally{$env:PYTHONPATH=$previous}
}
"PASS: atomic/idempotent boarding26 and$($variants.Count) custom-scope variants, preserved native click/credit/travel, actual flags/casts/root predicates and removal refresh; no client boarding/flight certification"
