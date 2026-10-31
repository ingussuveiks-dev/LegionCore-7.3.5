param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
$path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp'
$actor=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$old=(& git -C $repo show "ab9a5b3d9:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-acquisition actor missing'}
$normal=$actor.Replace('#include "sharasdal_acquisition.h"',(Block $old 'class go_art_sharasdal :')+';').Replace('    RegisterSpellScript(spell_sharasdal_acquisition);','').Replace('    RegisterAuraScript(aura_sharasdal_acquisition);','')
if(($normal -replace '\s','') -cne ($old -replace '\s','')){throw 'Other companions/heals/gossip/walls/Kraliss/Lady/registration changed'}
if($ScopeOnly){'PASS only old GO pickup replaced by scoped acquisition header and two matching loaders; all other actor content retained';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify repaired Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-acquisition$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
[IO.File]::WriteAllText("$out/Controller.inc",$(if($Baseline){(Block $old 'class go_art_sharasdal :')+';'}else{Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/sharasdal_acquisition.h" -Raw}))
foreach($name in @('Conversation.h','Item.h','InstanceScript.h','SpellAuraEffects.h')){[IO.File]::WriteAllText("$out/$name",'// Controlled native service boundary, not a linked game header.')}
$effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
foreach($name in @('ForceEquipItem','SendEvent','KillCredit')){[IO.File]::WriteAllText("$out/$name.inc",(Block $effects "void Spell::Effect$name("))}
[IO.File]::WriteAllText("$out/CriteriaTypes.inc",(Block (Get-Content "$repo/src/server/game/DataStores/DBCEnums.h" -Raw) 'enum CriteriaTypes')+';')
[IO.File]::WriteAllText("$out/NativeEnums.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'enum AuraRemoveMode')+';'+(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.h" -Raw) 'enum SceneEventStatus')+';')
[IO.File]::WriteAllText("$out/QuestObjectScope.inc",(Block (Get-Content "$repo/src/server/game/Entities/Object/QuestObjectScope.h" -Raw) 'struct QuestObjectScope')+';')
[IO.File]::WriteAllText("$out/BindQuestObject.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::BindQuestObject('))
[IO.File]::WriteAllText("$out/QuestLease.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::AddQuestDelayedEvent('))
$objectSource=Get-Content "$repo/src/server/game/Entities/Object/Object.cpp" -Raw
[IO.File]::WriteAllText("$out/BindScope.inc",(Block $objectSource 'void WorldObject::BindQuestObjectScope(')+(Block $objectSource 'bool WorldObject::IsQuestObjectExpired('))
$native=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw;$a=$native.IndexOf('void GameObject::SetLootState(');$b=$native.IndexOf('    if (m_model)',$a);[IO.File]::WriteAllText("$out/SetLootState.inc",$native.Substring($a,$b-$a)+"}`n")
[IO.File]::WriteAllText("$out/IsPersonal.inc",(Block $native 'bool GameObject::IsPersonal(')+(Block (Get-Content "$repo/src/server/game/Entities/GameObject/GameObjectData.cpp" -Raw) 'bool GameObjectTemplate::IsOploteChest('))
$release=Get-Content "$repo/src/server/game/Handlers/LootHandler.cpp" -Raw;$release=$release.Substring($release.IndexOf('        else if (loot->isLooted() || go->GetGoType() == GAMEOBJECT_TYPE_FISHINGNODE)'))
[IO.File]::WriteAllText("$out/ReleaseChest.inc",(Block $release '            if (go->GetGoType() == GAMEOBJECT_TYPE_CHEST)'))
$store=Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::StoreLootItem('
$a=$store.IndexOf('    ItemPosCountVec dest;');$b=$store.IndexOf('    if (qitem)',$a);[IO.File]::WriteAllText("$out/StoreInventory.inc",$store.Substring($a,$b-$a))
$a=$store.IndexOf('    if (!item->freeforall)');$b=$store.IndexOf('    --loot->unlootedCount;',$a);[IO.File]::WriteAllText("$out/ConsumeLoot.inc",$store.Substring($a,$b-$a)+'    --loot->unlootedCount;')
[IO.File]::WriteAllText("$out/IsLooted.inc",(Block (Get-Content "$repo/src/server/game/Loot/LootMgr.h" -Raw) 'bool isLooted() const'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''};[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SharasdalAcquisitionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Actual Sharasdal acquisition regression failed'}
if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$previous=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sharasdal_acquisition_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Actual acquisition Release implementation missing'}}finally{$env:PYTHONPATH=$previous}}
