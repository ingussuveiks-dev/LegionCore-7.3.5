param([switch]$VerifyReleaseBinary,[string]$MarksmanshipRowsPath,[switch]$MarksmanshipLoot,[switch]$MarksmanshipRoot,[switch]$MarksmanshipReturn,[switch]$MarksmanshipDialogue,[string]$TitanstrikeRowsPath,[switch]$TitanstrikeRoot,[switch]$TitanstrikeCast,[string]$XalatathRowsPath,[uint32]$XalatathLootId=247690,[switch]$XalatathRoot,[switch]$XalatathGossip,[switch]$XalatathBaseline,[string]$StromkarRowsPath,[uint32]$StromkarLootId=247877,[switch]$StromkarRoot,[string]$WarswordsRowsPath,[string]$EarthwarderRowsPath)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/talonclaw-pickup-admission"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_23_16_talonclaw_pickup_admission.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_go LIKE gameobject_template;
INSERT INTO saved_go SELECT * FROM gameobject_template WHERE entry=248834;
CREATE TEMPORARY TABLE gameobject_template LIKE saved_go;
INSERT INTO gameobject_template SELECT * FROM saved_go;
CREATE TEMPORARY TABLE saved_loot LIKE gameobject_loot_template;
INSERT INTO saved_loot SELECT * FROM gameobject_loot_template WHERE Entry=248834;
CREATE TEMPORARY TABLE gameobject_loot_template LIKE saved_loot;
INSERT INTO gameobject_loot_template SELECT * FROM saved_loot;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup IN(248834,248419);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=248834 AND SourceEntry=128808 AND SourceId=0;
$migration
$migration
SELECT 'count',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=248834 AND SourceEntry=128808 AND SourceId=0;
SELECT 'row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=248834 AND SourceEntry=128808 AND SourceId=0 ORDER BY ElseGroup,ConditionTypeOrReference;
SELECT 'other',COUNT(*) FROM conditions current JOIN saved_conditions old ON current.SourceTypeOrReferenceId=old.SourceTypeOrReferenceId AND current.SourceGroup=old.SourceGroup AND current.SourceEntry=old.SourceEntry AND current.SourceId=old.SourceId AND current.ElseGroup=old.ElseGroup AND current.ConditionTypeOrReference=old.ConditionTypeOrReference WHERE current.SourceGroup=248419;
SELECT 'saved-other',COUNT(*) FROM saved_conditions WHERE SourceGroup=248419;
SELECT 'object',COUNT(*) FROM gameobject_template current JOIN saved_go old USING(entry) WHERE current.Data0=old.Data0 AND current.Data1=old.Data1 AND current.flags=old.flags AND current.AIName=old.AIName AND current.ScriptName=old.ScriptName;
SELECT 'loot',COUNT(*) FROM gameobject_loot_template current JOIN saved_loot old USING(Entry,Item) WHERE current.Chance=old.Chance AND current.MinCount=old.MinCount AND current.MaxCount=old.MaxCount;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=248834 AND SourceEntry=128808 AND SourceId=0;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES(4,248834,128808,0,0,4,0,1,0,0,0,0,'','Custom condition');
$migration
SELECT 'custom',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=248834 AND SourceEntry=128808 AND SourceId=0;
SELECT 'custom-kept',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=248834 AND SourceEntry=128808 AND ConditionTypeOrReference=4 AND ConditionValue1=1;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=248834 AND SourceEntry=128808 AND SourceId=0;
UPDATE gameobject_template SET ScriptName='custom_pickup' WHERE entry=248834;
$migration
SELECT 'custom-no-add',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=248834 AND SourceEntry=128808;
UPDATE gameobject_template SET ScriptName='go_artifact_acquisition' WHERE entry=248834;
UPDATE gameobject_loot_template SET Reference=123 WHERE Entry=248834;
$migration
SELECT 'custom-loot',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=248834 AND SourceEntry=128808;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Pickup admission temporary SQL fixture failed'}
}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("count`t20","object`t1","loot`t1","custom`t1","custom-kept`t1","custom-no-add`t0","custom-loot`t0")){if($rows -notcontains $expected){throw "Pickup admission mismatch: $expected"}}
$other=($rows | Where-Object {$_ -like "other`t*"}) -replace '^other',''
$saved=($rows | Where-Object {$_ -like "saved-other`t*"}) -replace '^saved-other',''
if($other -ne $saved){throw 'Unrelated loot conditions changed'}
[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]@($rows | Where-Object {$_ -like "row`t*"} | ForEach-Object {$_.Substring(4)}))
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,$first);if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$cases=(Section $source '        case CONDITION_ITEM:' '        case CONDITION_ITEM_EQUIPPED:')+(Section $source '        case CONDITION_CLASS:' '        case CONDITION_RACE:')+(Section $source '        case CONDITION_QUESTREWARDED:' '        case CONDITION_QUEST_NONE:')+(Section $source '        case CONDITION_MAPID:' '        case CONDITION_AREAID:')+(Section $source '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:')
$cases+=(Section $source '        case CONDITION_ALIVE:' '        case CONDITION_HP_VAL:')
[IO.File]::WriteAllText("$out/Conditions.inc",$cases)
[IO.File]::WriteAllText("$out/ElseGroups.inc",(Section $source 'bool ConditionMgr::IsObjectMeetToConditionList(' 'bool ConditionMgr::IsObjectMeetToConditions(WorldObject* object,'))
[IO.File]::WriteAllText("$out/SmartEventConditions.inc",(Section $source 'bool ConditionMgr::IsObjectMeetingSmartEventConditions(' 'bool Condition::isLoaded() const'))
$dispatch=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/RootDispatch.inc",(Section $dispatch 'void SmartScript::ProcessEventsFor(' 'void SmartScript::ProcessAction('))
$source=Get-Content "$repo/src/server/game/Loot/LootMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/LootCondition.inc",(Section $source 'bool LootItem::AllowedForPlayer(Player const* player) const' 'void LootItem::AddAllowedLooter('))
$recheck=Section $source '    if (item && !AllowedForPlayer(player,' 'uint32 Loot::GetMaxSlotInLootFor('
[IO.File]::WriteAllText("$out/SlotRecheck.inc",$recheck.Substring(0,$recheck.LastIndexOf('}')))
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$store=Section $source 'void Player::StoreLootItem(' 'void Player::learnSpellHighRank('
if($store.IndexOf('loot->LootItemInSlot(') -lt 0 -or $store.IndexOf('CanStoreNewItem(') -lt 0 -or $store.IndexOf('loot->LootItemInSlot(') -ge $store.IndexOf('CanStoreNewItem(')){throw 'Native store validates slot after item storage'}
$source=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
$send=Section $source 'void Spell::SendLoot(' 'bool Spell::isDelayableNoMore()'
if($send.IndexOf('sScriptMgr->OnGossipHello(') -lt 0 -or $send.IndexOf('player->SendLoot(guid, loottype);') -lt 0 -or $send.IndexOf('sScriptMgr->OnGossipHello(') -ge $send.IndexOf('player->SendLoot(guid, loottype);')){throw 'Native open-lock loot bypasses the script admission'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($MarksmanshipLoot){'marksmanship-loot'}elseif($MarksmanshipRoot){'marksmanship-root'}elseif($MarksmanshipDialogue){'marksmanship-dialogue'}elseif($MarksmanshipReturn){'marksmanship-return'}else{'marksmanship'}
$fixture=if($MarksmanshipRowsPath){"`"$MarksmanshipRowsPath`" $mode"}else{"`"$out/conditions.tsv`""}
if($TitanstrikeRowsPath){$mode=if($TitanstrikeCast){'titanstrike-cast'}elseif($TitanstrikeRoot){'titanstrike-root'}else{'titanstrike-loot'};$fixture="`"$TitanstrikeRowsPath`" $mode"}
$source=Get-Content "$repo/src/server/game/Entities/GameObject/GameObjectData.cpp" -Raw
[IO.File]::WriteAllText("$out/GetLootId.inc",(Section $source 'uint32 GameObjectTemplate::GetLootId() const' 'uint32 GameObjectTemplate::GetGossipMenuId() const'))
if($XalatathRowsPath){
 $mode=if($XalatathGossip){'xalatath-gossip'}elseif($XalatathRoot){'xalatath-root'}else{'xalatath-loot'}
 if($XalatathBaseline){$mode+='-baseline'}
 $fixture="`"$XalatathRowsPath`" $mode $XalatathLootId"
}
if($StromkarRowsPath){$mode=if($StromkarRoot){'stromkar-root'}else{'stromkar-loot'};$fixture="`"$StromkarRowsPath`" $mode $StromkarLootId"}
if($WarswordsRowsPath){$fixture="`"$WarswordsRowsPath`" warswords-loot 248832"}
if($EarthwarderRowsPath){$fixture="`"$EarthwarderRowsPath`" earthwarder-loot 248831"}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/TalonclawPickupAdmissionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" $fixture`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native Talonclaw pickup admission failed'}
& "$PSScriptRoot/Test-ArtifactAcquisition.ps1"
if($VerifyReleaseBinary){
 $previousPython=$env:PYTHONPATH
 try {
  $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython"
  & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_talonclaw_pickup_admission_release.py" "$repo/build-extractors/bin/Release"
  if($LASTEXITCODE){throw 'Deployed pickup admission verification failed'}
 }finally{$env:PYTHONPATH=$previousPython}
}
'PASS: idempotent actual-schema source4 migration, custom/unrelated condition and loot preservation; native open-lock/store admission ordering'
