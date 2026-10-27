param([switch]$AddressSanitizer,[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/lights-wrath-pickup"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing native block $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){if($s[$b] -eq '{'){++$depth};if($s[$b] -eq '}'){--$depth};++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo,$out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 Copy-Item "$repo/src/server/scripts/Legion/ClassHalls/lights_wrath_pickup.h" "$out/Pickup.inc"
 $header=Get-Content "$repo/src/server/game/Entities/Player/Player.h" -Raw
 $unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw
 $auras=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuraDefines.h" -Raw
 [IO.File]::WriteAllText("$out/NativeEnums.inc",(Block $unit 'enum AuraRemoveMode')+';'+(Block $auras 'enum AuraEffectHandleModes')+';'+(Block $header 'enum SceneEventStatus')+';')
 [IO.File]::WriteAllText("$out/Pending.inc",(Block $header 'bool HasPendingTeleport() const'))
 [IO.File]::WriteAllText("$out/DelayScope.inc",(Block $header 'class ScopedTeleportDelay')+';')
 $player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
 [IO.File]::WriteAllText("$out/QuestDelay.inc",(Block $player 'void Player::AddQuestDelayedEvent('))
 [IO.File]::WriteAllText("$out/SceneSend.inc",(Block $player 'void Player::SendSpellScene('))
 [IO.File]::WriteAllText("$out/SceneCompleted.inc",(Block $player 'void Player::SceneCompleted('))
 [IO.File]::WriteAllText("$out/SceneStatus.inc",(Block $player 'bool Player::HasSceneStatus('))
 $a=$player.IndexOf('            SetDelayedTeleportFlag(IsCanDelayTeleport());',$player.IndexOf('// far teleport to another map'));$b=$player.IndexOf('            EndMovieMovement();',$a)
 if($a -lt 0 -or $b -le $a){throw 'Native far teleport deferral missing'}
 [IO.File]::WriteAllText("$out/TeleportDelay.inc",$player.Substring($a,$b-$a))
 $store=Block $player 'void Player::StoreLootItem('
 $a=$store.IndexOf('    ItemPosCountVec dest;');$b=$store.IndexOf('    if (qitem)',$a)
 [IO.File]::WriteAllText("$out/StoreInventory.inc",$store.Substring($a,$b-$a))
 $a=$store.IndexOf('    if (!item->freeforall)');$b=$store.IndexOf('    --loot->unlootedCount;',$a)
 [IO.File]::WriteAllText("$out/ConsumeLoot.inc",$store.Substring($a,$b-$a)+'    --loot->unlootedCount;')
 $native=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
 $a=$native.IndexOf('void GameObject::SetLootState(');$b=$native.IndexOf('    if (m_model)',$a)
 [IO.File]::WriteAllText("$out/SetLootState.inc",$native.Substring($a,$b-$a)+"}`n")
 [IO.File]::WriteAllText("$out/IsLooted.inc",(Block (Get-Content "$repo/src/server/game/Loot/LootMgr.h" -Raw) 'bool isLooted() const'))
 $release=Get-Content "$repo/src/server/game/Handlers/LootHandler.cpp" -Raw
 $match=[regex]::Match($release,'else if \((loot->isLooted\(\) \|\| go->GetGoType\(\) == GAMEOBJECT_TYPE_FISHINGNODE)\)')
 if(!$match.Success -or !$release.Contains('go->SetLootState(GO_JUST_DEACTIVATED, static_cast<Unit*>(player));')){throw 'Native completed loot release missing'}
 [IO.File]::WriteAllText("$out/CompleteLoot.inc",'return '+$match.Groups[1].Value+';')
 $effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
 [IO.File]::WriteAllText("$out/ForceEquip.inc",(Block $effects 'void Spell::EffectForceEquipItem('))
 [IO.File]::WriteAllText("$out/SendEvent.inc",(Block $effects 'void Spell::EffectSendEvent('))
 $s=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuras.cpp" -Raw
 [IO.File]::WriteAllText("$out/NativeDuration.inc",(Block (Block $s 'void Aura::Update(uint32 diff, Unit* caster)') 'if (m_duration > 0)'))
 $s=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw;$a=$s.IndexOf('// remove expired auras - do that after updates');$b=$s.IndexOf('// m_auraUpdateIterator can be updated',$a)
 [IO.File]::WriteAllText("$out/NativeExpiry.inc",$s.Substring($a,$b-$a))
 $s=Get-Content "$repo/src/server/game/Handlers/SceneHandler.cpp" -Raw
 [IO.File]::WriteAllText("$out/SceneCanceled.inc",(Block $s 'void WorldSession::HandleScenePlaybackCanceled('))
 [IO.File]::WriteAllText("$out/SceneCompletePacket.inc",(Block $s 'void WorldSession::HandleScenePlaybackComplete('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 foreach($route in @('priest','aluneth')){
  $define=if($route -eq 'aluneth'){'/DALUNETH'}else{''}
  [IO.File]::WriteAllText("$out/run-$route.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /W4 $asan $define /I`"$out`" `"$PSScriptRoot/LightsWrathPickupTest.cpp`" /Fe:$route.exe /Fo:$route.obj`nif errorlevel 1 exit /b 1`n$route.exe`nexit /b %errorlevel%`n")
  & "$out/run-$route.cmd";if($LASTEXITCODE){throw "Native $route pickup regression failed"}
 }
 if($VerifyNativeCatalog){
  $prior=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile LightsWrathCatalog.cpp -ProbeName LightsWrathCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
  $catalog=@(Get-Content "$out/native.tsv")
  foreach($row in @("DURATION`t208011`t9000`t9000","EFFECT`t208011`t0`t6`t430`t1244`t0`t1`t0","EFFECT`t208011`t2`t249`t0`t128868`t0`t1`t0","EFFECT`t208011`t3`t90`t0`t105109`t0`t1`t0","EFFECT`t218172`t1`t252`t0`t0`t0`t1`t17")){if($catalog -notcontains $row){throw 'Native pickup/scene/equip/return data contract differs'}}
  foreach($row in @("DURATION`t211954`t9000`t9000","EFFECT`t211954`t0`t6`t430`t1269`t0`t1`t0","EFFECT`t211954`t2`t249`t0`t127857`t0`t1`t0","EFFECT`t211954`t3`t90`t0`t105109`t0`t1`t0","CRITERION`t1101`t6`t47999`t30099`t36`t127857","CRITERION`t1101`t6`t48000`t29682`t92`t49875","CRITERION`t1101`t5`t47997`t34397`t92`t54145","CRITERION`t1101`t5`t55130`t34398`t92`t54146","CRITERION`t1101`t5`t55131`t34399`t92`t54147")){if($catalog -notcontains $row){throw 'Native Aluneth control/pickup/scene/equip/return data contract differs'}}
 }
}
