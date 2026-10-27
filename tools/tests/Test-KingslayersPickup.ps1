param([switch]$Baseline,[switch]$AddressSanitizer,[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/kingslayers-pickup"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/$(if($Baseline){'baseline'}else{'current'})" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp'
 $source=Get-Content "$repo/$path" -Raw
 if($Baseline){$source=(& git -C $repo show "8cb3e9cb:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pickup baseline missing'}}
 [IO.File]::WriteAllText("$out/Acquisition.inc",$(if($Baseline){''}else{Block $source 'namespace KingslayersAcquisition'}))
 [IO.File]::WriteAllText("$out/Callback.inc",(Block $source 'class go_the_kingslayers')+';')
 [IO.File]::WriteAllText("$out/CriteriaTypes.inc",(Block (Get-Content "$repo/src/server/game/DataStores/DBCEnums.h" -Raw) 'enum CriteriaTypes')+';')
 $native=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
 $first=$native.IndexOf('void GameObject::SetLootState(');$last=$native.IndexOf('    if (m_model)',$first)
 [IO.File]::WriteAllText("$out/SetLootState.inc",$native.Substring($first,$last-$first)+"}`n")
 [IO.File]::WriteAllText("$out/QuestQueue.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::AddQuestDelayedEvent('))
 $store=Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::StoreLootItem('
 $first=$store.IndexOf('    ItemPosCountVec dest;');$last=$store.IndexOf('    if (qitem)',$first)
 [IO.File]::WriteAllText("$out/StoreInventory.inc",$store.Substring($first,$last-$first))
 $first=$store.IndexOf('    if (!item->freeforall)');$last=$store.IndexOf('    --loot->unlootedCount;',$first)
 [IO.File]::WriteAllText("$out/ConsumeLoot.inc",$store.Substring($first,$last-$first)+'    --loot->unlootedCount;')
 [IO.File]::WriteAllText("$out/IsLooted.inc",(Block (Get-Content "$repo/src/server/game/Loot/LootMgr.h" -Raw) 'bool isLooted() const'))
 $release=Get-Content "$repo/src/server/game/Handlers/LootHandler.cpp" -Raw
 $predicate=[regex]::Match($release,'else if \((loot->isLooted\(\) \|\| go->GetGoType\(\) == GAMEOBJECT_TYPE_FISHINGNODE)\)')
 if(!$predicate.Success -or !$release.Contains('go->SetLootState(GO_JUST_DEACTIVATED, static_cast<Unit*>(player));')){throw 'Native completed release missing'}
 [IO.File]::WriteAllText("$out/CompleteLoot.inc",'return '+$predicate.Groups[1].Value+';')
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $definition=if($Baseline){'/DBASELINE'}else{''};if($AddressSanitizer){$definition+=' /fsanitize=address /Zi'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/KingslayersPickupTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Kingslayers pickup regression failed'}
 if($VerifyNativeCatalog){
  $previous=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile KingslayersPickupCatalog.cpp -ProbeName KingslayersPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native pickup catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
  $rows=Get-Content "$out/native.tsv"
  foreach($expected in @("EFFECT`t215112`t0`t6`t430`t1325`t0`t1`t0","EFFECT`t215113`t0`t249`t0`t128870`t0`t1`t0","CHILD`t128870`t128869`t16","DURATION`t215112`t15000`t15000","PERIOD`t215114`t1`t2000")){if($rows -notcontains $expected){throw "Native pickup mismatch: $expected"}}
 }
}
