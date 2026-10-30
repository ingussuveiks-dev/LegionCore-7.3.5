param([switch]$AddressSanitizer,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-full-bag-loot/run-$([DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fffffff'))"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 function Section($s,$a,$b){$i=$s.IndexOf($a);$j=$s.IndexOf($b,[Math]::Max(0,$i));if($i -lt 0 -or $j -le $i){throw "Missing native $a"};$s.Substring($i,$j-$i)}
 function Block($s,$a){$i=$s.IndexOf($a);if($i -lt 0){throw "Missing native $a"};$j=$s.IndexOf('{',$i)+1;$depth=1;while($depth){if($s[$j] -eq '{'){++$depth};if($s[$j] -eq '}'){--$depth};++$j};$s.Substring($i,$j-$i)}
 $player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
 $baseline=(git -C $repo show 'ad092752d:src/server/game/Entities/Player/Player.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned storage baseline unavailable'}
 [IO.File]::WriteAllText("$out/NativeCanEquip.inc",(Section $player 'InventoryResult Player::CanEquipItem(' 'InventoryResult Player::CanEquipChildItem('))
 [IO.File]::WriteAllText("$out/NativeSwap.inc",(Section $player 'void Player::SwapItem(' 'void Player::AddItemToBuyBackSlot('))
 [IO.File]::WriteAllText("$out/NativeOffhand.inc",(Section $player 'void Player::AutoUnequipOffhandIfNeed(' 'bool Player::HasItemFitToSpellRequirements('))
 $effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
 [IO.File]::WriteAllText("$out/NativeForceEquip.inc",(Block $effects 'void Spell::EffectForceEquipItem('))
 [IO.File]::WriteAllText("$out/NativeSimilarItems.inc",(Section $player 'InventoryResult Player::CanTakeMoreSimilarItems(' 'uint8 Player::GetItemLimitCategoryQuantity('))
 [IO.File]::WriteAllText("$out/NativeStoreNewItem.inc",(Section $player 'Item* Player::StoreNewItem(' 'std::vector<Item*> Player::GetItemListByEntry('))
 [IO.File]::WriteAllText("$out/BaselineStoreNewItem.inc",[regex]::Replace((Section $baseline 'Item* Player::StoreNewItem(' 'std::vector<Item*> Player::GetItemListByEntry('),'\bStoreNewItem\(','StoreNewItemBaseline('))
 foreach($kind in @('Native','Baseline')){
  $s=if($kind -eq 'Native'){$player}else{$baseline}
  $body=Section $s 'void Player::StoreLootItem(' 'uint8 Player::CalculateTalentsPoints('
  $permission=Section $body '    QuestItem* qitem' '    if (loot->shipmentBuildingType)'
  $grant=Section $body '    ItemPosCountVec dest;' '    if (ItemTemplate const* proto'
  $method="void Player::StoreLootItem(uint8 lootSlot,Loot*loot){`n$permission$grant`nsScriptMgr->OnLootItem(this,newitem,item->count);`n}"
  if($kind -eq 'Baseline'){$method=[regex]::Replace($method.Replace('StoreLootItem(','StoreLootItemBaseline('),'\bStoreNewItem\(','StoreNewItemBaseline(')}
  [IO.File]::WriteAllText("$out/${kind}StoreLootItem.inc",$method)
 }
 $scripts=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
 [IO.File]::WriteAllText("$out/LootActive.inc",(Section $scripts 'bool Active(Player*' 'bool Use(Player*'))
 [IO.File]::WriteAllText("$out/StromkarLootPolicy.inc",((Block $scripts 'class player_stromkar_loot_equipment')+';'))
 $s=Get-Content "$repo/src/server/game/Scripting/ScriptMgr.cpp" -Raw
 [IO.File]::WriteAllText("$out/LootPolicyDispatch.inc",(Block $s 'bool ScriptMgr::CanAutoEquipLoot('))
 $headers=@('src/server/game/Entities/Item/Item.h','src/server/game/Entities/Item/ItemTemplate.h','src/server/game/Entities/Player/Player.h','src/server/game/Miscellaneous/SharedDefines.h','src/server/game/DataStores/DBCEnums.h','src/server/game/Mails/Mail.h')|ForEach-Object{Get-Content "$repo/$_" -Raw}
 $enums=Get-Content "$repo/src/server/game/Entities/Object/Updates/UpdateFields.h" -Raw
 $seen=@{}
 foreach($token in @('EQUIP_ERR_OK','EQUIPMENT_SLOT_MAINHAND','INVENTORY_SLOT_BAG_START','CHILD_EQUIPMENT_SLOT_START','INVTYPE_2HWEAPON','ITEM_CLASS_WEAPON','ITEM_SUBCLASS_WEAPON_POLEARM','ITEM_SUBCLASS_AMMO_POUCH','ITEM_SUBCLASS_CONTAINER','ITEM_FLAG_CHILD','ITEM_FLAG3_ALWAYS_ALLOW_DUAL_WIELD','ITEM_MODIFIER_UPGRADE_ID','ITEM_LIMIT_CATEGORY_MODE_HAVE','MAIL_STATIONERY_GM','MAIL_CHECK_MASK_COPIED')){
  $found=$false
  foreach($header in $headers){$m=[regex]::Match($header,"\b$token\s*=");if(!$m.Success){continue};$start=$header.LastIndexOf('enum ',$m.Index);if($start -lt 0){continue};$tail=$header.Substring($start);$enum=(Block $tail ($tail.Substring(0,$tail.IndexOf('{')).Trim()))+';';$name=($enum -split '\{')[0];if(!$seen.ContainsKey($name)){$seen[$name]=$true;$enums+="`n$enum`n"};$found=$true;break}
  if(!$found){throw "Native enum for$token unavailable"}
 }
 [IO.File]::WriteAllText("$out/EquipmentEnums.inc",$enums)
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/StromkarFullBagLootTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native full-bag loot regression failed'}
 if($VerifyReleaseBinary){
  if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
  $priorPython=$env:PYTHONPATH
  try{
   $env:PYTHONPATH="$repo/.codex/pe-review-deps;$priorPython"
   $ErrorActionPreference='Continue'
   & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_stromkar_full_bag_release.py" $ReleaseDirectory > "$out/pe.stdout.log" 2> "$out/pe.stderr.log"
   $result=$LASTEXITCODE
   $ErrorActionPreference='Stop'
   Get-Content "$out/pe.stdout.log";Get-Content "$out/pe.stderr.log"
   if($result){throw 'Linked full-bag loot implementation failed'}
  }finally{$env:PYTHONPATH=$priorPython;$ErrorActionPreference='Stop'}
 }
}
