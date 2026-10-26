
param([string]$MigrationPath="$PSScriptRoot/../../sql/updates/world/2026_10_26_13_devourer_native_pickup.sql",[switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/devourer-pickup-data"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($out) -Test {
 $migration=Get-Content -LiteralPath $MigrationPath -Raw
 $header=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.h" -Raw
 if($header -notmatch 'CONDITION_SOURCE_TYPE_GAMEOBJECT_LOOT_TEMPLATE\s*=\s*4,'){throw 'Native GO loot source changed'}
 $tables=@('gameobject_template','gameobject_loot_template','conditions')
 $sql=''
 foreach($table in $tables){
  $filter=switch($table){'gameobject_template' {'entry IN(249347,249824)'} 'gameobject_loot_template' {'Entry IN(249347,249824)'} default {'SourceTypeOrReferenceId=4 AND SourceGroup IN(249347,249824)'}}
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $filter;CREATE TEMPORARY TABLE $table LIKE saved_$table;`n"
 }
 $sql+="DELETE FROM saved_conditions WHERE SourceGroup=249347 AND Comment LIKE 'Devourer native pickup - %';UPDATE saved_gameobject_template SET Data1=249347 WHERE entry=249347;"
 $reset=($tables|ForEach-Object {"DELETE FROM $_;INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $reset+='UPDATE gameobject_template SET Data1=0 WHERE entry=249347;'
 function Snap([string]$label){($tables|ForEach-Object {"SELECT '$label-$_',current.* FROM $_ current ORDER BY 2,3,4,5,6,7;"}) -join "`n"}
 $sql+="$reset`n$migration`n$(Snap 'first')`n$migration`n$(Snap 'second')`nSELECT 'rows',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=249347;SELECT 'link',Data0,Data1 FROM gameobject_template WHERE entry=249347;SELECT 'gate',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=249347 ORDER BY ElseGroup,ConditionTypeOrReference;"
 foreach($table in @('gameobject_template','gameobject_loot_template')){$sql+="SELECT 'expected-$table',current.* FROM saved_$table current ORDER BY 2,3,4,5,6,7;"}
 $variants=[ordered]@{
  'custom-script'="UPDATE gameobject_template SET ScriptName='custom' WHERE entry=249347;"
  'custom-link'='UPDATE gameobject_template SET Data1=123 WHERE entry=249347;'
  'custom-lock'='UPDATE gameobject_template SET Data0=1691 WHERE entry=249347;'
  'custom-loot'='UPDATE gameobject_loot_template SET Item=1 WHERE Entry=249347;'
  'custom-condition'='INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1,Comment) VALUES(4,249347,128476,15,1,NULL);'
  'custom-ai'="UPDATE gameobject_template SET AIName='SmartGameObjectAI' WHERE entry=249347;"
 }
 foreach($variant in $variants.Keys){$sql+="$reset`n$($variants[$variant])`n$(Snap "$variant-before")`n$migration`n$(Snap "$variant-after")"}
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$prior=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$db[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Devourer TEMP migration failed'}}finally{$env:MYSQL_PWD=$prior}
 [IO.File]::WriteAllLines("$out/fixture.tsv",[string[]]$rows)
 function Rows([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
 function Signature([string]$label){[string]::Join("`n",[string[]]@(Rows $label))}
 foreach($table in $tables){if((Signature "first-$table") -cne (Signature "second-$table")){throw "Non-idempotent migration: $table"};foreach($variant in $variants.Keys){if((Signature "$variant-before-$table") -cne (Signature "$variant-after-$table")){throw "Custom content changed: $variant $table"}}}
 foreach($table in @('gameobject_template','gameobject_loot_template')){if((Signature "expected-$table") -cne (Signature "first-$table")){throw "Native table changed beyond intended loot link: $table"}}
 if((Rows 'rows') -ne '14' -or (Rows 'link') -ne "43`t249347"){throw 'Missing native lock/loot14 rule result'}
 [IO.File]::WriteAllLines("$out/conditions.tsv",[string[]](Rows 'gate'))
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

 $source=Get-Content "$repo/src/server/game/Entities/GameObject/GameObjectData.cpp" -Raw
 [IO.File]::WriteAllText("$out/GetLootId.inc",(Section $source 'uint32 GameObjectTemplate::GetLootId() const' 'uint32 GameObjectTemplate::GetGossipMenuId() const'))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/DevourerPickupConditionsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/conditions.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Devourer GO loot condition regression failed'}
 'PASS TEMP migration repeated/full-template and loot comparison/six custom refusal variants; actual native GO source4/loot condition and cached slot regression. No live data or character change.'
}
