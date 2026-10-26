param([switch]$Baseline,[switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/tuure-native-pickup"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/$(if($Baseline){'baseline'}else{'current'})" -RuntimeDirectories @($repo,$out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $path='src/server/scripts/Scenario/Artifacts/Priest/Tuure/Tuure.cpp'
 $source=Get-Content "$repo/$path" -Raw
 if($Baseline){$source=(& git -C $repo show "d9137d5a:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pickup baseline missing'}}
 $current=Get-Content "$repo/$path" -Raw
 [IO.File]::WriteAllText("$out/Acquisition.inc",(Block $current 'namespace TuureAcquisition'))
 [IO.File]::WriteAllText("$out/Callback.inc",(Block $source 'class go_art_tuure :')+';')
 [IO.File]::WriteAllText("$out/CastState.inc",(Block $current 'namespace TuureCast')+(Block $current 'class spell_tuure_pickup_cast :')+';'+(Block $current 'class spell_tuure_departure :')+';')
 [IO.File]::WriteAllText("$out/Presentation.inc",(Block $current 'namespace TuurePresentation'))
 $player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
 [IO.File]::WriteAllText("$out/QuestDelay.inc",(Block $player 'void Player::AddQuestDelayedEvent('))
 $header=Get-Content "$repo/src/server/game/Entities/Player/Player.h" -Raw
 [IO.File]::WriteAllText("$out/Pending.inc",(Block $header 'bool HasPendingTeleport() const'))
 [IO.File]::WriteAllText("$out/DelayScope.inc",(Block $header 'class ScopedTeleportDelay')+';')
 $first=$player.IndexOf('            SetDelayedTeleportFlag(IsCanDelayTeleport());',$player.IndexOf('// far teleport to another map'));$last=$player.IndexOf('            EndMovieMovement();',$first)
 if($first -lt 0 -or $last -le $first){throw 'Native far deferral missing'}
 [IO.File]::WriteAllText("$out/TeleportDelay.inc",$player.Substring($first,$last-$first))
 $effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
 [IO.File]::WriteAllText("$out/ForceEquip.inc",(Block $effects 'void Spell::EffectForceEquipItem('))
 [IO.File]::WriteAllText("$out/SendEvent.inc",(Block $effects 'void Spell::EffectSendEvent('))
 $fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
 [IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
 [IO.File]::WriteAllText("$out/CriteriaTypes.inc",(Block (Get-Content "$repo/src/server/game/DataStores/DBCEnums.h" -Raw) 'enum CriteriaTypes')+';')
 $native=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
 $first=$native.IndexOf('void GameObject::SetLootState(');$last=$native.IndexOf('    if (m_model)',$first)
 [IO.File]::WriteAllText("$out/SetLootState.inc",$native.Substring($first,$last-$first)+"}`n")
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
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/TuureNativePickupTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Tuure pickup regression failed'}

}
