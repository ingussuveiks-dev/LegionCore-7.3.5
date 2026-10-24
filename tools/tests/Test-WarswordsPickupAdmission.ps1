param([switch]$Baseline,[switch]$VerifyReleaseBinary,[switch]$LootBaseline,[switch]$ReturnBaseline,[switch]$DepartureBaseline)
$ErrorActionPreference='Stop'
if(($Baseline -or $LootBaseline -or $ReturnBaseline -or $DepartureBaseline) -and $VerifyReleaseBinary){throw 'Baseline cannot verify repaired Release'}
if(@($Baseline,$LootBaseline,$ReturnBaseline,$DepartureBaseline|Where-Object {$_}).Count -gt 1){throw 'Choose one pinned baseline'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-pickup-admission"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Warrior/Warswords/Warswords.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline -or $LootBaseline -or $ReturnBaseline -or $DepartureBaseline){$revision=if($DepartureBaseline){'d7c3ac3b'}elseif($ReturnBaseline){'6c6a6fc7'}elseif($LootBaseline){'c3728f9e'}else{'c9b90aba'};$source=(& git -C $repo show "${revision}:$path") -join "`n";if($LASTEXITCODE){throw 'Missing pickup baseline'}}
[IO.File]::WriteAllText("$out/PickupObject.inc",(Block $source 'class go_art_warswords :')+';')
$helper=if($Baseline){''}else{Block $source 'namespace WarswordsAcquisition'}
[IO.File]::WriteAllText("$out/PickupAdmission.inc",$helper)
$native=Get-Content "$repo/src/server/game/Scenarios/Scenario.cpp" -Raw
[IO.File]::WriteAllText("$out/ScenarioCompleted.inc",(Block $native 'bool Scenario::IsCompleted(').Replace('Scenario::',''))
$criteria=Get-Content "$repo/src/server/game/DataStores/DBCEnums.h" -Raw
[IO.File]::WriteAllText("$out/CriteriaTypes.inc",(Block $criteria 'enum CriteriaTypes')+';')
$native=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$store=Block $native 'void Player::StoreLootItem('
$first=$store.IndexOf('    ItemPosCountVec dest;');$last=$store.IndexOf('    if (qitem)',$first)
if($first -lt 0 -or $last -le $first){throw 'Missing native inventory admission/storage'}
[IO.File]::WriteAllText("$out/NativeStoreInventory.inc",$store.Substring($first,$last-$first))
$first=$store.IndexOf('    if (!item->freeforall)');$last=$store.IndexOf('    --loot->unlootedCount;',$first)
[IO.File]::WriteAllText("$out/NativeConsumeLoot.inc",$store.Substring($first,$last-$first)+'    --loot->unlootedCount;')
$native=Get-Content "$repo/src/server/game/Loot/LootMgr.h" -Raw
[IO.File]::WriteAllText("$out/NativeIsLooted.inc",(Block $native 'bool isLooted() const'))
$native=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$first=$native.IndexOf('void GameObject::SetLootState(');$last=$native.IndexOf('    if (m_model)',$first)
[IO.File]::WriteAllText("$out/NativeSetLootState.inc",$native.Substring($first,$last-$first)+"}`n")
$native=Get-Content "$repo/src/server/game/Handlers/LootHandler.cpp" -Raw
$predicate=[regex]::Match($native,'else if \((loot->isLooted\(\) \|\| go->GetGoType\(\) == GAMEOBJECT_TYPE_FISHINGNODE)\)')
if(!$predicate.Success -or !$native.Contains('go->SetLootState(GO_JUST_DEACTIVATED, static_cast<Unit*>(player));')){throw 'Missing native completed-loot release'}
[IO.File]::WriteAllText("$out/NativeCompleteLoot.inc",'return '+$predicate.Groups[1].Value+';')
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$baselineArgument=if($DepartureBaseline){' /DDEPARTURE_BASELINE'}elseif($ReturnBaseline){' /DRETURN_BASELINE'}elseif($LootBaseline){' /DBASELINE /DLOOT_BASELINE'}elseif($Baseline){' /DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4$baselineArgument /I`"$out`" `"$PSScriptRoot/WarswordsPickupAdmissionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Warswords native pickup admission failed'}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$connection[3]
  $live=@("SELECT 'binding',entry,ScriptName,AIName FROM gameobject_template WHERE ScriptName='go_art_warswords';SELECT 'spawn',guid,map,PhaseId FROM gameobject WHERE id=248832;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE){throw 'Live weapon binding read failed'}
 }finally{$env:MYSQL_PWD=$previous}
 if($live.Count -ne 2 -or $live -notcontains "binding`t248832`tgo_art_warswords`t" -or $live -notcontains "spawn`t134150`t1511`t7001"){throw 'Unexpected weapon binding/spawn context'}
 [IO.File]::WriteAllLines("$out/live.tsv",[string[]]$live)
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_warswords_pickup_admission_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical pickup admission failed'}}finally{$env:PYTHONPATH=$previous}
}
