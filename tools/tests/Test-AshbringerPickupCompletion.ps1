param([switch]$Baseline,[switch]$CapacityBaseline,[switch]$AddressSanitizer,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if(($Baseline -or $CapacityBaseline) -and $VerifyReleaseBinary){throw 'Pinned baseline cannot verify fixed Release'}
if($Baseline -and $CapacityBaseline){throw 'Select one baseline'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/ashbringer-pickup-completion"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/broken_shore.cpp'
$source=Get-Content "$repo/$path" -Raw
$pinned=(& git -C $repo show "85b4f0c2:$path") -join "`n"
if($LASTEXITCODE){throw 'Pinned pickup baseline unavailable'}
if($Baseline){$source=$pinned}elseif($CapacityBaseline){$source=(& git -C $repo show "bf251a6d:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned capacity baseline unavailable'}}else{
 $removed='instance->DoUpdateAchievementCriteria(CRITERIA_TYPE_COMPLETE_SCENARIO, 775, 1);'
 $capacity=Block (Block $source 'class go_bs_the_ashbringer :') 'if (!player->HasItemCount(120978))'
 if(($source.Replace($capacity,'') -replace '\s','') -cne ($pinned.Replace($removed,'') -replace '\s','')){throw 'Unrelated acquisition behavior changed beyond completion credit and scoped capacity preflight'}
}
[IO.File]::WriteAllText("$out/Pickup.inc",(Block (Block $source 'class go_bs_the_ashbringer :') 'void OnLootStateChanged(').Replace(' override',''))
[IO.File]::WriteAllText("$out/Constants.inc",(Block (Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/broken_shore.h" -Raw) 'enum Data')+';'+(Block $source 'enum eSpells')+';'+(Block (Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.h" -Raw) 'enum LootState')+';')
[IO.File]::AppendAllText("$out/Constants.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'enum InventorySlot')+';'+(Block (Get-Content "$repo/src/server/game/Entities/Item/Item.h" -Raw) 'enum InventoryResult')+';')
[IO.File]::WriteAllText("$out/CapacityBridge.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.h" -Raw) 'InventoryResult CanStoreNewItem('))
$native=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$first=$native.IndexOf('void GameObject::SetLootState(');$last=$native.IndexOf('    if (m_model)',$first)
if($first -lt 0 -or $last -le $first){throw 'Native loot state dispatcher missing'}
[IO.File]::WriteAllText("$out/SetLootState.inc",$native.Substring($first,$last-$first)+'}')
$criteria=Get-Content "$repo/src/server/game/DataStores/DBCEnums.h" -Raw
foreach($entry in @(@('SCRIPT_EVENT_2',92),@('COMPLETE_SCENARIO',152),@('COMPLETE_SCENARIO_COUNT',151))){if($criteria -notmatch "CRITERIA_TYPE_$($entry[0])\s*=\s*$($entry[1])\s*,"){throw 'Native criterion constants changed'}}
[IO.File]::WriteAllText("$out/InstanceCredit.inc",(Block (Get-Content "$repo/src/server/game/Instances/InstanceScript.cpp" -Raw) 'void InstanceScript::DoUpdateAchievementCriteria(').Replace('CriteriaTypes type','uint32 type'))
$scenario=Get-Content "$repo/src/server/game/Scenarios/Scenario.cpp" -Raw
[IO.File]::WriteAllText("$out/Completion.inc",((Block $scenario 'bool Scenario::IsCompleted(')+(Block $scenario 'uint8 Scenario::GetStepCount(')).Replace('Scenario::',''))
$reward=Block $scenario 'void Scenario::Reward('
$credits=[regex]::Match($reward,'player->UpdateAchievementCriteria\(CRITERIA_TYPE_COMPLETE_SCENARIO, scenarioId, 1\);\s*player->UpdateAchievementCriteria\(CRITERIA_TYPE_COMPLETE_SCENARIO_COUNT, 1\);').Value
if(!$credits -or (Block $scenario 'void Scenario::UpdateCurrentStep(') -notmatch 'if \(IsCompleted\(false\)\)\s*Reward\(false, oldStep\);'){throw 'Native natural completion reward flow changed'}
[IO.File]::WriteAllText("$out/RewardCredits.inc",$credits)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=$(if($Baseline){'/DBASELINE '}elseif($CapacityBaseline){'/DCAPACITY_BASELINE '}else{''})+$(if($AddressSanitizer){'/fsanitize=address /Zi'}else{''})
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/AshbringerPickupCompletionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native pickup completion regression failed'}
if($Baseline -or $CapacityBaseline){return}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $rows=@("SET SESSION TRANSACTION READ ONLY;SELECT 'binding',entry,type,Data0,Data1,Data8,Data11,Data15,Data17,Data26,Data30,Data31,AIName,ScriptName FROM gameobject_template WHERE entry=247318;SELECT 'spawn',guid,id,map,phaseId FROM gameobject WHERE id=247318;SELECT 'loot',Entry,Item,Chance,MinCount,MaxCount FROM gameobject_loot_template WHERE Entry=247318;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Read-only pickup query failed'}
}finally{$env:MYSQL_PWD=$previous}
if(Compare-Object @("binding`t247318`t3`t1691`t0`t0`t0`t0`t32543`t0`t0`t0`t`tgo_bs_the_ashbringer","spawn`t128117`t247318`t1500`t5942","loot`t247318`t120978`t100`t1`t1") $rows){throw 'Existing pickup/runtime loot metadata changed'}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile AshbringerPickupCatalog.cpp -ProbeName AshbringerPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"
if(!$? -or $LASTEXITCODE){throw 'Native pickup catalog failed'}
$expected=@(
 "SPELL`t180850`tRetrieving the Ashbringer","EFFECT`t180850`t0`t90`t0`t114505`t0`t1`t0","EFFECT`t180850`t1`t61`t0`t47621`t0`t0`t0",
 "EFFECT`t180850`t2`t64`t0`t0`t199827`t0`t0","EFFECT`t180850`t3`t64`t0`t0`t186987`t0`t0","EFFECT`t180850`t4`t64`t0`t0`t192145`t0`t0","EFFECT`t180850`t5`t64`t0`t0`t190777`t0`t0","EFFECT`t180850`t6`t6`t23`t0`t181257`t1`t0",
 "SPELL`t181257`tRetrieving The Ashbringer","EFFECT`t181257`t0`t249`t0`t120978`t0`t1`t0","EFFECT`t181257`t1`t61`t0`t46506`t0`t0`t0","EFFECT`t181257`t2`t64`t0`t0`t225875`t1`t0",
 "SPELL`t181511`tThe Ashbringer","EFFECT`t181511`t0`t249`t0`t120978`t0`t1`t0",
 "STEP`t1`t1485`t42486`t0","STEP`t2`t1486`t45098`t0","STEP`t3`t1487`t45145`t0","STEP`t0`t1488`t42545`t0","STEP`t5`t1853`t45269`t0","STEP`t6`t1854`t45277`t0","STEP`t4`t2632`t49026`t0",
 "CRITERION`t45146`t45145`t28823`t92`t46130","CRITERION`t45792`t45145`t29022`t92`t46506","CRITERION`t45278`t45277`t28906`t92`t46162")
if(Compare-Object $expected (Get-Content "$out/native.tsv")){throw 'Native seven-step/retrieval metadata changed'}
Write-Host 'PASS unchanged live247318 binding/zero loot link and native retrieval180850/181257 effect249 plus seven nonbonus775 steps; no item-first gate or new weapon grant'
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_ashbringer_pickup_completion_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed pickup completion guard missing'}}finally{$env:PYTHONPATH=$previous}
}
