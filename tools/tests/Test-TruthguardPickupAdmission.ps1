param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned baseline cannot verify current Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/truthguard-pickup-admission"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/shields_rest.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "aa988ac6:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Truthguard callback unavailable'}}
[IO.File]::WriteAllText("$out/Policy.inc",$(if($Baseline){''}else{Block $source 'namespace TruthguardAcquisition'}))
[IO.File]::WriteAllText("$out/Callback.inc",(Block $source 'class go_sr_truthguard')+';')
$native=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$first=$native.IndexOf('void GameObject::SetLootState(');$last=$native.IndexOf('    if (m_model)',$first)
[IO.File]::WriteAllText("$out/NativeSetLootState.inc",$native.Substring($first,$last-$first)+"}`n")
$store=Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::StoreLootItem('
$first=$store.IndexOf('    ItemPosCountVec dest;');$last=$store.IndexOf('    if (qitem)',$first)
if($first -lt 0 -or $last -le $first){throw 'Missing native storage fragment'}
[IO.File]::WriteAllText("$out/NativeStoreInventory.inc",$store.Substring($first,$last-$first))
$first=$store.IndexOf('    if (!item->freeforall)');$last=$store.IndexOf('    --loot->unlootedCount;',$first)
[IO.File]::WriteAllText("$out/NativeConsumeLoot.inc",$store.Substring($first,$last-$first)+'    --loot->unlootedCount;')
[IO.File]::WriteAllText("$out/NativeIsLooted.inc",(Block (Get-Content "$repo/src/server/game/Loot/LootMgr.h" -Raw) 'bool isLooted() const'))
$release=Get-Content "$repo/src/server/game/Handlers/LootHandler.cpp" -Raw
$predicate=[regex]::Match($release,'else if \((loot->isLooted\(\) \|\| go->GetGoType\(\) == GAMEOBJECT_TYPE_FISHINGNODE)\)')
if(!$predicate.Success -or !$release.Contains('go->SetLootState(GO_JUST_DEACTIVATED, static_cast<Unit*>(player));')){throw 'Native completed-loot release missing'}
[IO.File]::WriteAllText("$out/NativeCompleteLoot.inc",'return '+$predicate.Groups[1].Value+';')
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$out`" `"$PSScriptRoot/TruthguardPickupAdmissionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Truthguard admission host failed'}
if($Baseline){return}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'binding',entry,type,displayId,flags,Data0,Data1,Data14,Data30,Data31,AIName,ScriptName FROM gameobject_template WHERE ScriptName='go_sr_truthguard';
SELECT 'spawn',guid,id,map,PhaseId FROM gameobject WHERE id=249420;
SELECT 'loot',entry,item,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount FROM gameobject_loot_template WHERE entry=249420;
SELECT 'gates',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=249420;
SELECT 'smart',COUNT(*) FROM smart_scripts WHERE source_type=1 AND entryorguid IN(249420,-140818);
SELECT 'objective',ID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=42017 ORDER BY ID;
'@
 $previousPassword=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$rows=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Truthguard binding metadata read failed'}}finally{$env:MYSQL_PWD=$previousPassword}
 $expected=@("binding`t249420`t3`t9806`t2113536`t1691`t249420`t23645`t0`t0`t`tgo_sr_truthguard","spawn`t140818`t249420`t1495`t6383","loot`t249420`t128866`t100`t0`t1`t0`t1`t1","gates`t14","smart`t0","objective`t283388`t0`t0`t105889`t1`t0","objective`t283389`t0`t1`t105891`t1`t34","objective`t283477`t0`t2`t105892`t1`t2")
 if(Compare-Object $expected $rows){throw 'Truthguard metadata changed; review native real-loot/admission contract'}
 $rows|Set-Content -Encoding utf8 "$out/live.tsv"
 & "$PSScriptRoot/Test-TruthguardSurrender.ps1" -VerifyReleaseBinary
 if($LASTEXITCODE){throw 'Truthguard stage/native/installed surrender changed'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_truthguard_pickup_admission_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed Truthguard admission missing'}}finally{$env:PYTHONPATH=$previous}
}
