param([string]$MigrationPath="$PSScriptRoot/../../sql/updates/world/2026_10_26_15_tuure_native_pickup.sql",[switch]$AddressSanitizer,[switch]$VerifyLive)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/tuure-native-pickup-data"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo,$out) -Test {
 $migration=Get-Content -LiteralPath $MigrationPath -Raw
 if((Get-Content "$repo/src/server/game/Conditions/ConditionMgr.h" -Raw) -notmatch 'CONDITION_SOURCE_TYPE_GAMEOBJECT_LOOT_TEMPLATE\s*=\s*4,'){throw 'Native GO loot source changed'}
 $filters=[ordered]@{
  gameobject_template='entry IN(249705,249824)'
  gameobject_loot_template='Entry IN(249705,249824)'
  conditions='SourceTypeOrReferenceId=4 AND SourceGroup IN(249705,249824)'
  spell_script_names='ABS(spell_id) IN(210733,210734,210818,205040)'
  spell_scene='MiscValue=1265'
  spell_scene_event='MiscValue=1265'
  spell_target_position='id IN(210818,205040)'
 }
 $setup=''
 foreach($table in $filters.Keys){$setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"}
 $setup+="UPDATE saved_gameobject_template SET Data1=249705 WHERE entry=249705;UPDATE gameobject_template SET Data1=0 WHERE entry=249705;DELETE FROM conditions WHERE SourceGroup=249705 AND Comment LIKE 'Tuure native pickup - %';DELETE FROM spell_script_names WHERE ABS(spell_id) IN(210733,210734,210818) AND ScriptName IN('spell_tuure_pickup_cast','spell_tuure_departure');"
 function Snapshot([string]$label){($filters.Keys|ForEach-Object {"SELECT '$label-$_',current.* FROM $_ current;"}) -join "`n"}
 function Rows([string[]]$rows,[string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
 function Signature([string[]]$rows,[string]$label){(Rows $rows $label|Sort-Object -CaseSensitive) -join "`n"}
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$prior=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$db[3]
  $sql="$setup`n$migration`n$(Snapshot 'first')`n$migration`n$(Snapshot 'second')`nSELECT 'rows',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=249705;SELECT 'link',Data0,Data1 FROM gameobject_template WHERE entry=249705;SELECT 'gate',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=249705 ORDER BY ElseGroup,ConditionTypeOrReference;SELECT 'bindings',COUNT(*) FROM spell_script_names WHERE (spell_id IN(210733,210734) AND ScriptName='spell_tuure_pickup_cast') OR (spell_id=210818 AND ScriptName='spell_tuure_departure');"
  foreach($table in @('gameobject_template','gameobject_loot_template','spell_scene','spell_scene_event','spell_target_position')){$sql+="SELECT 'expected-$table',current.* FROM saved_$table current;"}
  $rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE){throw 'Tuure ordinary TEMP migration failed'}
  foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Non-idempotent Tuure migration: $table"}}
  foreach($table in @('gameobject_template','gameobject_loot_template','spell_scene','spell_scene_event','spell_target_position')){if((Signature $rows "expected-$table") -cne (Signature $rows "first-$table")){throw "Native/custom neighboring content changed: $table"}}
  foreach($expected in @("rows`t14","link`t1691`t249705","bindings`t3")){if($rows -notcontains $expected){throw 'Missing native Tuure link/conditions/hooks'}}
  [IO.File]::WriteAllLines("$out/idempotent.tsv",[string[]]$rows)
  [IO.File]::WriteAllLines("$out/conditions.tsv",[string[]](Rows $rows 'gate'))
  $variants=[ordered]@{
   script="UPDATE gameobject_template SET ScriptName='custom' WHERE entry=249705;"
   link='UPDATE gameobject_template SET Data1=123 WHERE entry=249705;'
   lock='UPDATE gameobject_template SET Data0=43 WHERE entry=249705;'
   loot='UPDATE gameobject_loot_template SET Item=1 WHERE Entry=249705;'
   condition='INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1,Comment) VALUES(4,249705,128825,15,1,NULL);'
   ai="UPDATE gameobject_template SET AIName='custom' WHERE entry=249705;"
   scene="UPDATE spell_scene SET ScriptName='custom' WHERE MiscValue=1265;"
   event="INSERT INTO spell_scene_event VALUES(1265,'complete',1,0,'custom');"
   binding="INSERT INTO spell_script_names VALUES(210734,'custom');"
   destination='UPDATE spell_target_position SET target_map=1 WHERE id=210818;'
   missing='DELETE FROM spell_target_position WHERE id=210818;'
  }
  foreach($name in $variants.Keys){
   $sql="$setup`n$($variants[$name])`n$(Snapshot 'before')`n$migration`n$(Snapshot 'after')"
   # Expected failing counterfactual only: continue after the explicit45000 refusal to inspect TEMP state.
   $stderr="$out/expected-$name-stderr.txt"
   $fixture=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names --force 2> $stderr)
   $errorText=Get-Content $stderr -Raw
   if($errorText -notmatch '1644 \(45000\)' -or $errorText -notmatch 'Tuure native pickup custom/missing' -or @($errorText -split "`n"|Where-Object {$_ -match '^ERROR '}).Count -ne 1){throw "Unexpected TEMP refusal diagnostic: $name"}
   foreach($table in $filters.Keys){if((Signature $fixture "before-$table") -cne (Signature $fixture "after-$table")){throw "Refused Tuure migration changed $table in $name"}}
   [IO.File]::WriteAllLines("$out/expected-$name.tsv",[string[]]$fixture)
   "PASS expected custom/missing counterfactual refusal: $name; all seven TEMP tables unchanged"
  }
  if($VerifyLive){
   $sha=[Security.Cryptography.SHA1]::Create();try{$hash=[BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes(([IO.File]::ReadAllText((Resolve-Path $MigrationPath))).Replace("`r`n","`n")))).Replace('-','')}finally{$sha.Dispose()}
   $live=@("SELECT 'update',hash,state FROM updates WHERE name='2026_10_26_15_tuure_native_pickup.sql';SELECT 'link',Data0,Data1,ScriptName FROM gameobject_template WHERE entry=249705;SELECT 'conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=249705;SELECT 'binding',spell_id,ScriptName FROM spell_script_names WHERE ABS(spell_id) IN(210733,210734,210818);"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names)
   if($LASTEXITCODE){throw 'Live Tuure pickup readback failed'}
   foreach($expected in @("update`t$hash`tRELEASED","link`t1691`t249705`tgo_art_tuure","conditions`t14","binding`t210733`tspell_tuure_pickup_cast","binding`t210734`tspell_tuure_pickup_cast","binding`t210818`tspell_tuure_departure")){if($live -notcontains $expected){throw 'Live Tuure native pickup/hash mismatch'}}
   'PASS actual updater LF-text hash/RELEASED state/native lock/loot and three live spell registrations'
  }
 }finally{$env:MYSQL_PWD=$prior}

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
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/TuurePickupConditionsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`" `"$out/conditions.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Tuure GO loot condition regression failed'}
 'PASS TEMP migration repeated/full-template and loot comparison/eleven explicit custom refusal variants; actual native GO source4/loot condition and cached slot regression. No live data or character change.'
}
