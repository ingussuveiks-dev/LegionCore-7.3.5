param([switch]$Baseline,[switch]$LootBaseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if(($Baseline -or $LootBaseline) -and $VerifyReleaseBinary){throw 'Baseline cannot certify current Release'}
if($Baseline -and $LootBaseline){throw 'Select one baseline'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/silver-hand-pickup-admission"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline -or $LootBaseline){$revision=if($LootBaseline){'7af29f67'}else{'b7b06b71'};$source=(& git -C $repo show "${revision}:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Silver Hand baseline missing'}}
[IO.File]::WriteAllText("$out/Admission.inc",$(if($Baseline){''}else{Block $source 'namespace SilverHandAcquisition'}))
[IO.File]::WriteAllText("$out/Callback.inc",(Block $source 'class go_art_silverhand')+';')
$enums=Get-Content "$repo/src/server/game/DataStores/DBCEnums.h" -Raw
[IO.File]::WriteAllText("$out/CriteriaTypes.inc",(Block $enums 'enum CriteriaTypes')+';')
$native=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$first=$native.IndexOf('void GameObject::SetLootState(');$last=$native.IndexOf('    if (m_model)',$first)
if($first -lt 0 -or $last -lt $first){throw 'Native state dispatcher missing'}
[IO.File]::WriteAllText("$out/SetLootState.inc",$native.Substring($first,$last-$first)+"}`n")
$native=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$store=Block $native 'void Player::StoreLootItem('
$first=$store.IndexOf('    ItemPosCountVec dest;');$last=$store.IndexOf('    if (qitem)',$first)
if($first -lt 0 -or $last -le $first){throw 'Native storage fragment missing'}
[IO.File]::WriteAllText("$out/StoreInventory.inc",$store.Substring($first,$last-$first))
$first=$store.IndexOf('    if (!item->freeforall)');$last=$store.IndexOf('    --loot->unlootedCount;',$first)
if($first -lt 0 -or $last -le $first){throw 'Native loot consumption missing'}
[IO.File]::WriteAllText("$out/ConsumeLoot.inc",$store.Substring($first,$last-$first)+'    --loot->unlootedCount;')
[IO.File]::WriteAllText("$out/IsLooted.inc",(Block (Get-Content "$repo/src/server/game/Loot/LootMgr.h" -Raw) 'bool isLooted() const'))
$release=Get-Content "$repo/src/server/game/Handlers/LootHandler.cpp" -Raw
$predicate=[regex]::Match($release,'else if \((loot->isLooted\(\) \|\| go->GetGoType\(\) == GAMEOBJECT_TYPE_FISHINGNODE)\)')
if(!$predicate.Success -or !$release.Contains('go->SetLootState(GO_JUST_DEACTIVATED, static_cast<Unit*>(player));')){throw 'Native completed loot release missing'}
[IO.File]::WriteAllText("$out/CompleteLoot.inc",'return '+$predicate.Groups[1].Value+';')
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}elseif($LootBaseline){'/DLOOT_BASELINE'}else{'/DCURRENT_LOOT'}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/SilverHandPickupAdmissionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Silver Hand pickup admission failed'}
if($VerifyReleaseBinary){
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SilverHandPickupCatalog.cpp -ProbeName SilverHandPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native Silver Hand catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $rows=Get-Content "$out/native.tsv"
 foreach($expected in @("MAP_DUNGEON`t1539`t1","CRITERION`t10`t30111`t92`t50966`t0","ITEM`t128823`t48`t17`tThe Silver Hand","STEP_REWARD`t10`t0","PICKUP_MISC`t211838`t1`t105`t1056","PICKUP_DURATION`t105`t9000`t9000`t0","PICKUP_CAST_TIME`t1`t0`t0`t0")){if($rows -notcontains $expected){throw "Native pickup scope changed: $expected"}}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $password=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$rows=@("SET SESSION TRANSACTION READ ONLY;SELECT 'binding',entry,AIName,ScriptName FROM gameobject_template WHERE entry=249824;SELECT 'script-count',COUNT(*) FROM gameobject_template WHERE ScriptName='go_art_silverhand';SELECT 'spawn',guid,id,map,PhaseId FROM gameobject WHERE id=249824;SELECT 'smart',COUNT(*) FROM smart_scripts WHERE entryorguid IN(249824,-134279) AND source_type=1;SELECT 'shared',ScenarioID,MapID,Class FROM scenario_data WHERE MapID=1539 ORDER BY ScenarioID;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Read-only pickup binding query failed'}}finally{$env:MYSQL_PWD=$password}
 foreach($expected in @("binding`t249824`t`tgo_art_silverhand","script-count`t1","spawn`t134279`t249824`t1539`t7004","smart`t0","shared`t991`t1539`t5","shared`t1037`t1539`t1","shared`t1092`t1539`t2")){if($rows -notcontains $expected){throw "Live pickup scope changed: $expected"}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_pickup_admission_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed Silver Hand pickup admission missing'}}finally{$env:PYTHONPATH=$previous}
}
