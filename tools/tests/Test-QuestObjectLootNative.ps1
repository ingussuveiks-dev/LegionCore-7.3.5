param([switch]$Baseline,[string]$OutputDirectory='',[string]$BaselineDirectory='',
      [switch]$TakingPrecautions,[switch]$Collector,[switch]$Scenes,[switch]$Kyle,[switch]$Seasoning,[switch]$SeasoningBaseline,
      [switch]$Aquantion,[switch]$AquantionBaseline,[switch]$Anowyn,[switch]$AnowynBaseline,
      [switch]$AlliedCity,[switch]$AlliedCityBaseline,[switch]$Duskwither,[switch]$DuskwitherBaseline,
      [switch]$RedSnapper,[switch]$RedSnapperBaseline,[switch]$SwiftDiscipline,[switch]$SwiftDisciplineBaseline,
      [switch]$ExtinguishingHope,[switch]$ExtinguishingHopeBaseline,
      [switch]$MulgoreCages,[switch]$MulgoreCagesBaseline,
      [switch]$AshenvaleFelWood,[switch]$AshenvaleFelWoodBaseline,
      [switch]$ElwynnQuests,[switch]$ElwynnQuestsBaseline,
      [switch]$SupplyChests,[switch]$SupplyChestsBaseline,
      [switch]$ForestHeart,[switch]$ForestHeartBaseline,[switch]$DarkmoonStrength,[switch]$Tortusk,[switch]$ClusterCluck,[switch]$PoolPony,[switch]$MinerTroubles,[switch]$DreadHead,[switch]$CaravanDelivery,[switch]$WebbedResearcher,[switch]$LiFeiDuel,[switch]$ShangXiEscort,[switch]$YurivPendant,[switch]$FelfireCanister,[switch]$LifebringerSapling,[switch]$LordamereCannon,[switch]$TitansTerminal,[switch]$StillpinePrimer,[switch]$StillpinePrimerBaseline,[switch]$MonkeyBusiness,[switch]$MonkeyBusinessBaseline,[switch]$MonkeyBusinessFlightBaseline,[switch]$HogtiedRazormane,[switch]$HogtiedRazormaneBaseline,[switch]$TongaGuide,[switch]$TongaGuideBaseline,[switch]$AirfieldFirefighting,[switch]$AirfieldFirefightingBaseline,[switch]$FelfireSeedlings,[switch]$FelfireSeedlingsBaseline,[switch]$BattleboarTroughs,[switch]$BattleboarTroughsBaseline,[switch]$BalboaBasalt,[switch]$BalboaBasaltBaseline,[switch]$VileFinLeash,[switch]$VileFinLeashBaseline,[switch]$Absorbent,[switch]$AbsorbentBaseline,[switch]$SfgMinefield,[switch]$SfgMinefieldBaseline,[switch]$Dozercism,[switch]$DozercismBaseline,[switch]$StagnantOasis,[switch]$StagnantOasisBaseline,[switch]$AmaniCatacombs,[switch]$AmaniCatacombsBaseline,[switch]$RavagerCage,[switch]$RavagerCageBaseline,[switch]$AkidaCages,[switch]$AkidaCagesBaseline,[switch]$PrincessStillpine,[switch]$PrincessStillpineBaseline,[switch]$BloodcupBraid,[switch]$BloodcupBraidBaseline,[switch]$SamophlangeManual,[switch]$SamophlangeManualBaseline,[switch]$GutsAndGore,[switch]$GutsAndGoreBaseline,[string]$FixtureConfig='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$scopedProbes = @(@($TakingPrecautions,$Collector,$Scenes,$Kyle,$Seasoning,$SeasoningBaseline,$Aquantion,$AquantionBaseline,$Anowyn,$AnowynBaseline,$AlliedCity,$AlliedCityBaseline,$Duskwither,$DuskwitherBaseline,$RedSnapper,$RedSnapperBaseline,$SwiftDiscipline,$SwiftDisciplineBaseline,$ExtinguishingHope,$ExtinguishingHopeBaseline,$MulgoreCages,$MulgoreCagesBaseline,$AshenvaleFelWood,$AshenvaleFelWoodBaseline,$ElwynnQuests,$ElwynnQuestsBaseline,$SupplyChests,$SupplyChestsBaseline,$ForestHeart,$ForestHeartBaseline,$DarkmoonStrength,$Tortusk,$ClusterCluck,$PoolPony,$MinerTroubles,$DreadHead,$CaravanDelivery,$WebbedResearcher,$LiFeiDuel,$ShangXiEscort,$YurivPendant,$FelfireCanister,$LifebringerSapling,$LordamereCannon,$TitansTerminal,$StillpinePrimer,$StillpinePrimerBaseline,$MonkeyBusiness,$MonkeyBusinessBaseline,$MonkeyBusinessFlightBaseline,$HogtiedRazormane,$HogtiedRazormaneBaseline,$TongaGuide,$TongaGuideBaseline,$AirfieldFirefighting,$AirfieldFirefightingBaseline,$FelfireSeedlings,$FelfireSeedlingsBaseline,$BattleboarTroughs,$BattleboarTroughsBaseline,$BalboaBasalt,$BalboaBasaltBaseline,$VileFinLeash,$VileFinLeashBaseline,$Absorbent,$AbsorbentBaseline,$SfgMinefield,$SfgMinefieldBaseline,$Dozercism,$DozercismBaseline,$StagnantOasis,$StagnantOasisBaseline,$AmaniCatacombs,$AmaniCatacombsBaseline,$RavagerCage,$RavagerCageBaseline,$AkidaCages,$AkidaCagesBaseline,$PrincessStillpine,$PrincessStillpineBaseline,$BloodcupBraid,$BloodcupBraidBaseline,$SamophlangeManual,$SamophlangeManualBaseline,$GutsAndGore,$GutsAndGoreBaseline) | Where-Object { $_ }).Count
if($scopedProbes -and ($Baseline -or !$FixtureConfig -or $scopedProbes -gt 1)){throw 'Use one named probe and its private migrated fixture'}
if(!$OutputDirectory -and $DarkmoonStrength){$OutputDirectory="$repo/build-extractors/runtime-test/quest-object-loot-darkmoon-strength"}
if(!$OutputDirectory -and $Tortusk){$OutputDirectory="$repo/build-extractors/runtime-test/quest-tortusk"}
if(!$OutputDirectory -and $ClusterCluck){$OutputDirectory="$repo/build-extractors/runtime-test/quest-cluster-cluck"}
if(!$OutputDirectory -and $PoolPony){$OutputDirectory="$repo/build-extractors/runtime-test/quest-pool-pony"}
if(!$OutputDirectory -and $MinerTroubles){$OutputDirectory="$repo/build-extractors/runtime-test/quest-miner-troubles"}
if(!$OutputDirectory -and $DreadHead){$OutputDirectory="$repo/build-extractors/runtime-test/quest-dread-head"}
if(!$OutputDirectory -and $CaravanDelivery){$OutputDirectory="$repo/build-extractors/runtime-test/quest-caravan-delivery"}
if(!$OutputDirectory -and $WebbedResearcher){$OutputDirectory="$repo/build-extractors/runtime-test/quest-webbed-researcher"}
if(!$OutputDirectory -and $LiFeiDuel){$OutputDirectory="$repo/build-extractors/runtime-test/quest-li-fei-duel"}
if(!$OutputDirectory -and $ShangXiEscort){$OutputDirectory="$repo/build-extractors/runtime-test/quest-shang-xi-escort"}
if(!$OutputDirectory -and $YurivPendant){$OutputDirectory="$repo/build-extractors/runtime-test/quest-yuriv-pendant"}
if(!$OutputDirectory -and $FelfireCanister){$OutputDirectory="$repo/build-extractors/runtime-test/quest-felfire-canister"}
if(!$OutputDirectory -and $LifebringerSapling){$OutputDirectory="$repo/build-extractors/runtime-test/quest-lifebringer-sapling"}
if(!$OutputDirectory -and $LordamereCannon){$OutputDirectory="$repo/build-extractors/runtime-test/quest-lordamere-cannon"}
if(!$OutputDirectory -and $TitansTerminal){$OutputDirectory="$repo/build-extractors/runtime-test/quest-titans-terminal"}
if(!$OutputDirectory -and ($GutsAndGore -or $GutsAndGoreBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-guts-and-gore"}
if(!$OutputDirectory -and ($SamophlangeManual -or $SamophlangeManualBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-samophlange-manual"}
if(!$OutputDirectory -and ($BloodcupBraid -or $BloodcupBraidBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-bloodcup-braid"}
if(!$OutputDirectory -and ($PrincessStillpine -or $PrincessStillpineBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-princess-stillpine"}
if(!$OutputDirectory -and ($AkidaCages -or $AkidaCagesBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-akida-cages"}
if(!$OutputDirectory -and ($RavagerCage -or $RavagerCageBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-ravager-cage"}
if(!$OutputDirectory -and ($AmaniCatacombs -or $AmaniCatacombsBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-amani-catacombs"}
if(!$OutputDirectory -and ($StagnantOasis -or $StagnantOasisBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-stagnant-oasis"}
if(!$OutputDirectory -and ($Dozercism -or $DozercismBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-dozercism"}
if(!$OutputDirectory -and ($SfgMinefield -or $SfgMinefieldBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-sfg-minefield"}
if(!$OutputDirectory -and ($Absorbent -or $AbsorbentBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-absorbent"}
if(!$OutputDirectory -and ($VileFinLeash -or $VileFinLeashBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-vile-fin-leash"}
if(!$OutputDirectory -and ($BalboaBasalt -or $BalboaBasaltBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-balboa-basalt"}
if(!$OutputDirectory -and ($BattleboarTroughs -or $BattleboarTroughsBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-battleboar-troughs"}
if(!$OutputDirectory -and ($FelfireSeedlings -or $FelfireSeedlingsBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-felfire-seedlings"}
if(!$OutputDirectory -and ($AirfieldFirefighting -or $AirfieldFirefightingBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-airfield-firefighting"}
if(!$OutputDirectory -and ($TongaGuide -or $TongaGuideBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-tonga-guide"}
if(!$OutputDirectory -and ($HogtiedRazormane -or $HogtiedRazormaneBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-hogtied-razormane"}
if(!$OutputDirectory -and ($MonkeyBusiness -or $MonkeyBusinessBaseline -or $MonkeyBusinessFlightBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-monkey-business"}
if(!$OutputDirectory -and ($StillpinePrimer -or $StillpinePrimerBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-stillpine-primer"}
if(!$OutputDirectory -and ($ForestHeart -or $ForestHeartBaseline)){
 $OutputDirectory="$repo/build-extractors/runtime-test/quest-object-loot-forest-heart$(if($ForestHeartBaseline){'-counterfactual'})"
}
if(!$OutputDirectory -and ($SupplyChests -or $SupplyChestsBaseline)){
 $OutputDirectory="$repo/build-extractors/runtime-test/quest-object-loot-supply-chests$(if($SupplyChestsBaseline){'-counterfactual'})"
}
if(!$OutputDirectory -and ($ElwynnQuests -or $ElwynnQuestsBaseline)){
 $OutputDirectory="$repo/build-extractors/runtime-test/quest-object-loot-elwynn$(if($ElwynnQuestsBaseline){'-counterfactual'})"
}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/quest-object-loot$(if($Baseline){'-baseline'}elseif($TakingPrecautions){'-taking-precautions'}elseif($Collector){'-collector'}elseif($Scenes){'-scenes'}elseif($Kyle){'-kyle'}elseif($Seasoning){'-seasoning'}elseif($SeasoningBaseline){'-seasoning-counterfactual'}elseif($Aquantion){'-aquantion'}elseif($AquantionBaseline){'-aquantion-counterfactual'}elseif($Anowyn){'-anowyn'}elseif($AnowynBaseline){'-anowyn-counterfactual'}elseif($AlliedCity){'-allied-city'}elseif($AlliedCityBaseline){'-allied-city-counterfactual'}elseif($Duskwither){'-duskwither'}elseif($DuskwitherBaseline){'-duskwither-counterfactual'}elseif($RedSnapper){'-red-snapper'}elseif($RedSnapperBaseline){'-red-snapper-counterfactual'}elseif($SwiftDiscipline){'-swift-discipline'}elseif($SwiftDisciplineBaseline){'-swift-discipline-counterfactual'}elseif($ExtinguishingHope){'-extinguishing-hope'}elseif($ExtinguishingHopeBaseline){'-extinguishing-hope-counterfactual'}elseif($MulgoreCages){'-mulgore-cages'}elseif($MulgoreCagesBaseline){'-mulgore-cages-counterfactual'}elseif($AshenvaleFelWood){'-ashenvale-fel-wood'}elseif($AshenvaleFelWoodBaseline){'-ashenvale-fel-wood-counterfactual'})"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup|Where-Object Condition -like '*Release|x64*'|Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';')|Where-Object {$_ -and $_ -notlike '%*'}
$libs=$group.Link.AdditionalDependencies.Split(';')|Where-Object {$_ -and $_ -notlike '%*'}|ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$game=$libs|Where-Object {$_ -match 'game[\\/]Release[\\/]game.lib$'}|Select-Object -First 1
$scriptsLibrary=$libs|Where-Object {$_ -match 'scripts[\\/]Release[\\/]scripts.lib$'}|Select-Object -First 1
if(!$game -or !$scriptsLibrary){throw 'Missing actual final-link game or scripts library'}
if($Baseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/quest-loot-baseline-330996949"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '330996949' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned counterfactual library hash differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($MonkeyBusinessBaseline -or $MonkeyBusinessFlightBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/kezan-monkey-flight-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '2dc07336c' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned banana counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($SamophlangeManualBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/samophlange-manual-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne 'faf9ec251' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Samophlange counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($BloodcupBraidBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/quest-item-combine-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '66d415b15' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Bloodcup Braid counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($PrincessStillpineBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/princess-stillpine-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '977154cfb' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game -or
    (Get-FileHash "$BaselineDirectory/scripts.lib").Hash -ne $manifest.scripts){throw 'Pinned Princess Stillpine counterfactual libraries differ'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}elseif($_ -eq $scriptsLibrary){"$BaselineDirectory/scripts.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
 $scriptsLibrary="$BaselineDirectory/scripts.lib"
}elseif($AkidaCagesBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/akida-cages-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '0788b380c' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game -or
    (Get-FileHash "$BaselineDirectory/scripts.lib").Hash -ne $manifest.scripts){throw 'Pinned Akida cage counterfactual libraries differ'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}elseif($_ -eq $scriptsLibrary){"$BaselineDirectory/scripts.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
 $scriptsLibrary="$BaselineDirectory/scripts.lib"
}elseif($RavagerCageBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/ravager-cage-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '96b4a8e74' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game -or
    (Get-FileHash "$BaselineDirectory/scripts.lib").Hash -ne $manifest.scripts){throw 'Pinned Ravager Cage counterfactual libraries differ'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}elseif($_ -eq $scriptsLibrary){"$BaselineDirectory/scripts.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
 $scriptsLibrary="$BaselineDirectory/scripts.lib"
}elseif($AmaniCatacombsBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/amani-catacombs-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne 'face8a49c' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Amani Catacombs counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($StagnantOasisBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/stagnant-oasis-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne 'cfce801ab' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Stagnant Oasis counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($DozercismBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/dozercism-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne 'ae0cd9135' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Dozercism counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($SfgMinefieldBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/in-the-face-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne 'c826f0dc2' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned SFG counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($AbsorbentBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/azshara-quest-kits-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '77c06293c' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Absorbent counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($VileFinLeashBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/ever-so-lonely-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '5ae0117d5' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Murloc Leash counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($BalboaBasaltBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/balboa-basalt-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '15bc6e838' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Balboa counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($BattleboarTroughsBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/battleboar-troughs-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '98284209e' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Throw Torch counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($FelfireSeedlingsBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/felfire-seedlings-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne 'c523f0338' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned planting counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($TongaGuideBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/tonga-guide-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '8f17181c8' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned summit counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($HogtiedRazormaneBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/razormane-hogtie-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '1d9557196' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Grol''dom counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif(!$AlliedCityBaseline){
 foreach($file in @('Loot/LootMgr.cpp','Loot/LootMgr.h','Entities/Object/Object.h','Entities/Player/Player.cpp','Handlers/LootHandler.cpp')){
  if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
 }
}
$questSource=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
$query=[regex]::Match($questSource,'(?s)void QuestDataStoreMgr::LoadQuests\(\).*?WorldDatabase.Query\(("SELECT ".*?" FROM quest_template")\);').Groups[1].Value
if(!$query){throw 'Native quest loader SELECT missing'}
[IO.File]::WriteAllText("$out/NativeLostArmyQuest.inc",$query+' " WHERE ID IN(44923,45088)"')
$probeSource = "$PSScriptRoot/QuestObjectLootNative.cpp"
if($TakingPrecautions){
 $probeSource = "$PSScriptRoot/HallowsEndTakingPrecautionsNative.cpp"
 [IO.File]::WriteAllText("$out/NativeTakingPrecautionsQuest.inc",$query+' " WHERE ID IN(29402,29426)"')
}
if($Collector){
 $probeSource = "$PSScriptRoot/HallowsEndCollectorNative.cpp"
 [IO.File]::WriteAllText("$out/NativeCollectorQuest.inc",$query+' " WHERE ID IN(29403,29411,29413,29427,29428,29429)"')
}
if($Scenes){
 $probeSource = "$PSScriptRoot/HallowsEndScenesNative.cpp"
 [IO.File]::WriteAllText("$out/NativeHallowsEndScenesQuest.inc",$query+' " WHERE ID IN(29399,29411,29425,29428)"')
}
if($Kyle){
 $probeSource = "$PSScriptRoot/KyleLunchNative.cpp"
 [IO.File]::WriteAllText("$out/NativeKyleQuest.inc",$query+' " WHERE ID=11129"')
}
if($Seasoning -or $SeasoningBaseline){
 $probeSource = "$PSScriptRoot/SeasoningRacksNative.cpp"
 [IO.File]::WriteAllText("$out/NativeSeasoningQuest.inc",$query+' " WHERE ID=9275"')
}
if($Aquantion -or $AquantionBaseline){
 $probeSource = "$PSScriptRoot/AquantionAltarNative.cpp"
 [IO.File]::WriteAllText("$out/NativeAquantionQuest.inc",$query+' " WHERE ID=9174"')
}
if($Anowyn -or $AnowynBaseline){
 $probeSource = "$PSScriptRoot/AnowynCrystalNative.cpp"
 [IO.File]::WriteAllText("$out/NativeAnowynQuest.inc",$query+' " WHERE ID=9169"')
}
if($AlliedCity -or $AlliedCityBaseline){
 $probeSource = "$PSScriptRoot/AlliedRaceCityBoardNative.cpp"
 [IO.File]::WriteAllText("$out/NativeAlliedCityQuest.inc",$query+' " WHERE ID IN (49772,49773,49788,49933,50303,50305,50313,50319)"')
}
if($Duskwither -or $DuskwitherBaseline){
 $probeSource = "$PSScriptRoot/DuskwitherSpireNative.cpp"
 [IO.File]::WriteAllText("$out/NativeDuskwitherQuest.inc",$query+' " WHERE ID=8889"')
}
if($RedSnapper -or $RedSnapperBaseline){
 $probeSource = "$PSScriptRoot/RedSnapperNative.cpp"
 [IO.File]::WriteAllText("$out/NativeRedSnapperQuest.inc",$query+' " WHERE ID=9452"')
}
if($SwiftDiscipline -or $SwiftDisciplineBaseline){
 $probeSource = "$PSScriptRoot/SwiftDisciplineNative.cpp"
 [IO.File]::WriteAllText("$out/NativeSwiftDisciplineQuest.inc",$query+' " WHERE ID=9066"')
}
if($ExtinguishingHope -or $ExtinguishingHopeBaseline){
 $probeSource = "$PSScriptRoot/ExtinguishingHopeNative.cpp"
 [IO.File]::WriteAllText("$out/NativeExtinguishingHopeQuest.inc",$query+' " WHERE ID IN (26391,28817,28818,28819,28820,28821,28822,28823,29083,31145)"')
}
if($MulgoreCages -or $MulgoreCagesBaseline){
 $probeSource = "$PSScriptRoot/MulgoreCagesNative.cpp"
 [IO.File]::WriteAllText("$out/NativeMulgoreCagesQuest.inc",$query+' " WHERE ID IN (14452,24852,14458)"')
}
if($AshenvaleFelWood -or $AshenvaleFelWoodBaseline){
 $probeSource = "$PSScriptRoot/AshenvaleFelWoodNative.cpp"
 [IO.File]::WriteAllText("$out/NativeAshenvaleFelWoodQuest.inc",$query+' " WHERE ID IN (26453,26454)"')
}
if($ElwynnQuests -or $ElwynnQuestsBaseline){
 $probeSource = "$PSScriptRoot/ElwynnQuestRoutesNative.cpp"
 [IO.File]::WriteAllText("$out/NativeElwynnQuests.inc",$query+' " WHERE ID IN (11,35,37,40,45,46,52,71,239,5545)"')
}
if($SupplyChests -or $SupplyChestsBaseline){
 $probeSource = "$PSScriptRoot/QuestSupplyChestsNative.cpp"
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($ForestHeart -or $ForestHeartBaseline){
 $probeSource = "$PSScriptRoot/ForestHeartNative.cpp"
 [IO.File]::WriteAllText("$out/NativeForestHeartQuest.inc",$query+' " WHERE ID=13796"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($DarkmoonStrength){
 $probeSource = "$PSScriptRoot/DarkmoonStrengthNative.cpp"
 [IO.File]::WriteAllText("$out/NativeDarkmoonStrength.inc",$query+' " WHERE ID=29433"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 foreach($file in @('Entities/Player/KillRewarder.cpp','Entities/Unit/Unit.cpp','Spells/Spell.cpp','Spells/SpellEffects.cpp','World/World.cpp')){
  if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
 }
}
if($Tortusk){
 $probeSource = "$PSScriptRoot/TortuskNative.cpp"
 [IO.File]::WriteAllText("$out/NativeTortusk.inc",$query+' " WHERE ID=13968"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($ClusterCluck){
 $probeSource = "$PSScriptRoot/ClusterCluckNative.cpp"
 [IO.File]::WriteAllText("$out/NativeClusterCluck.inc",$query+' " WHERE ID=24671"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 $scripts=$libs|Where-Object {$_ -match 'scripts[\\/]Release[\\/]scripts.lib$'}|Select-Object -First 1
 if(!$scripts -or (Get-Item "$repo/src/server/scripts/Maelstrom/LostIsles/lost_isles.cpp").LastWriteTimeUtc -gt (Get-Item $scripts).LastWriteTimeUtc){throw 'Actual scripts library predates the clucker change'}
}
if($PoolPony){
 $probeSource = "$PSScriptRoot/PoolPonyNative.cpp"
 [IO.File]::WriteAllText("$out/NativePoolPony.inc",$query+' " WHERE ID=24864"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 $scripts=$libs|Where-Object {$_ -match 'scripts[\\/]Release[\\/]scripts.lib$'}|Select-Object -First 1
 if(!$scripts -or (Get-Item "$repo/src/server/scripts/Maelstrom/LostIsles/lost_isles.cpp").LastWriteTimeUtc -gt (Get-Item $scripts).LastWriteTimeUtc){throw 'Actual scripts library predates the hatchling change'}
}
if($MinerTroubles){
 $probeSource = "$PSScriptRoot/MinerTroublesNative.cpp"
 [IO.File]::WriteAllText("$out/NativeMinerTroubles.inc",$query+' " WHERE ID=14021"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 $scripts=$libs|Where-Object {$_ -match 'scripts[\\/]Release[\\/]scripts.lib$'}|Select-Object -First 1
 if(!$scripts -or (Get-Item "$repo/src/server/scripts/Maelstrom/LostIsles/lost_isles.cpp").LastWriteTimeUtc -gt (Get-Item $scripts).LastWriteTimeUtc){throw 'Actual scripts library predates the miner change'}
 if((Get-Item "$repo/src/server/game/Maps/Map.cpp").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw 'Actual game library predates the scoped miner change'}
}
if($DreadHead){
 $probeSource = "$PSScriptRoot/DreadHeadNative.cpp"
 [IO.File]::WriteAllText("$out/NativeDreadHead.inc",$query+' " WHERE ID=13842"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 $scripts=$libs|Where-Object {$_ -match 'scripts[\\/]Release[\\/]scripts.lib$'}|Select-Object -First 1
 if(!$scripts -or (Get-Item "$repo/src/server/scripts/Kalimdor/zone_ashenvale.cpp").LastWriteTimeUtc -gt (Get-Item $scripts).LastWriteTimeUtc){throw 'Actual scripts library predates the Durak change'}
}
if($CaravanDelivery){
 $probeSource = "$PSScriptRoot/CaravanDeliveryNative.cpp"
 [IO.File]::WriteAllText("$out/NativeCaravanDelivery.inc",$query+' " WHERE ID=13975"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 $scripts=$libs|Where-Object {$_ -match 'scripts[\\/]Release[\\/]scripts.lib$'}|Select-Object -First 1
 if(!$scripts -or (Get-Item "$repo/src/server/scripts/Kalimdor/zone_the_barrens.cpp").LastWriteTimeUtc -gt (Get-Item $scripts).LastWriteTimeUtc){throw 'Actual scripts library predates the delivery change'}
 foreach($file in @('Maps/Map.cpp','Entities/Player/Player.cpp')){
  if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates the delivery change in $file"}
 }
}
if($WebbedResearcher){
 $probeSource = "$PSScriptRoot/WebbedResearcherNative.cpp"
 [IO.File]::WriteAllText("$out/NativeWebbedResearcher.inc",$query+' " WHERE ID=9670"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 $scripts=$libs|Where-Object {$_ -match 'scripts[\\/]Release[\\/]scripts.lib$'}|Select-Object -First 1
 if(!$scripts -or (Get-Item "$repo/src/server/scripts/Kalimdor/zone_bloodmyst_isle.cpp").LastWriteTimeUtc -gt (Get-Item $scripts).LastWriteTimeUtc){throw 'Actual scripts library predates the webbed-creature implementation'}
}
if($LiFeiDuel){
 $probeSource = "$PSScriptRoot/LiFeiDuelNative.cpp"
 [IO.File]::WriteAllText("$out/NativeLiFeiDuel.inc",$query+' " WHERE ID=29421"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 $scripts=$libs|Where-Object {$_ -match 'scripts[\\/]Release[\\/]scripts.lib$'}|Select-Object -First 1
 if(!$scripts -or (Get-Item "$repo/src/server/scripts/Pandaria/zone_the_wandering_isle.cpp").LastWriteTimeUtc -gt (Get-Item $scripts).LastWriteTimeUtc){throw 'Actual scripts library predates the Li Fei change'}
}
if($ShangXiEscort){
 $probeSource = "$PSScriptRoot/ShangXiEscortNative.cpp"
 [IO.File]::WriteAllText("$out/NativeShangXiEscort.inc",$query+' " WHERE ID=29787"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 $scripts=$libs|Where-Object {$_ -match 'scripts[\\/]Release[\\/]scripts.lib$'}|Select-Object -First 1
 if(!$scripts -or (Get-Item "$repo/src/server/scripts/Pandaria/zone_the_wandering_isle.cpp").LastWriteTimeUtc -gt (Get-Item $scripts).LastWriteTimeUtc){throw 'Actual scripts library predates the Shang Xi change'}
 if((Get-Item "$repo/src/server/game/Maps/Map.cpp").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw 'Actual game library predates the scoped Shang Xi summon'}
}
if($YurivPendant){
 $probeSource = "$PSScriptRoot/YurivPendantNative.cpp"
 [IO.File]::WriteAllText("$out/NativeYurivPendant.inc",$query+' " WHERE ID=264"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if((Get-Item "$repo/src/server/game/Spells/SpellMgr.cpp").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw 'Actual game library predates the pendant lock correction'}
}
if($FelfireCanister){
 $probeSource = "$PSScriptRoot/FelfireCanisterNative.cpp"
 [IO.File]::WriteAllText("$out/NativeFelfireCanister.inc",$query+' " WHERE ID=13730"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 $scripts=$libs|Where-Object {$_ -match 'scripts[\\/]Release[\\/]scripts.lib$'}|Select-Object -First 1
 if(!$scripts -or (Get-Item "$repo/src/server/scripts/Kalimdor/zone_ashenvale.cpp").LastWriteTimeUtc -gt (Get-Item $scripts).LastWriteTimeUtc){throw 'Actual scripts library predates the Felfire channel change'}
}
if($LifebringerSapling){
 $probeSource = "$PSScriptRoot/LifebringerSaplingNative.cpp"
 [IO.File]::WriteAllText("$out/NativeLifebringerSapling.inc",$query+' " WHERE ID=13925"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if((Get-Item "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw 'Actual game library predates the sapling credit change'}
}
if($LordamereCannon){
 $probeSource = "$PSScriptRoot/LordamereCannonNative.cpp"
 [IO.File]::WriteAllText("$out/NativeLordamereCannon.inc",$query+' " WHERE ID=27232"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 $scripts=$libs|Where-Object {$_ -match 'scripts[\\/]Release[\\/]scripts.lib$'}|Select-Object -First 1
 if(!$scripts -or (Get-Item "$repo/src/server/scripts/EasternKingdoms/zone_silverpine_forest.cpp").LastWriteTimeUtc -gt (Get-Item $scripts).LastWriteTimeUtc){throw 'Actual scripts library predates the Lordamere cannon'}
 if((Get-Item "$repo/src/server/game/Maps/Map.cpp").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw 'Actual game library predates the accepted scope for the Lordamere waves'}
}
if($TitansTerminal){
 $probeSource = "$PSScriptRoot/TitansTerminalNative.cpp"
 [IO.File]::WriteAllText("$out/NativeTitansTerminal.inc",$query+' " WHERE ID=13918"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 $scripts=$libs|Where-Object {$_ -match 'scripts[\\/]Release[\\/]scripts.lib$'}|Select-Object -First 1
 if(!$scripts -or (Get-Item "$repo/src/server/scripts/Kalimdor/zone_darkshore.cpp").LastWriteTimeUtc -gt (Get-Item $scripts).LastWriteTimeUtc){throw 'Actual scripts library predates the buried marker selection'}
 foreach($file in @('AI/SmartScripts/SmartScript.cpp','Entities/Object/Object.cpp')){
  if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
 }
}
if($BattleboarTroughs -or $BattleboarTroughsBaseline){
 $probeSource = "$PSScriptRoot/BattleboarTroughsNative.cpp"
 [IO.File]::WriteAllText("$out/NativeBattleboarTroughs.inc",$query+' " WHERE ID=14461"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($BattleboarTroughs -and (Get-Item "$repo/src/server/game/Spells/Spell.cpp").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw 'Actual game library predates the torch hit fix'}
}
if($FelfireSeedlings -or $FelfireSeedlingsBaseline){
 $probeSource = "$PSScriptRoot/FelfireSeedlingsNative.cpp"
 [IO.File]::WriteAllText("$out/NativeFelfireSeedlings.inc",$query+' " WHERE ID IN (9526,26446)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($FelfireSeedlings){
  foreach($file in @('Handlers/SpellHandler.cpp','Spells/Spell.cpp','Entities/GameObject/GameObject.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($AirfieldFirefighting -or $AirfieldFirefightingBaseline){
 $probeSource = "$PSScriptRoot/AirfieldFirefightingNative.cpp"
 [IO.File]::WriteAllText("$out/NativeAirfieldFirefighting.inc",$query+' " WHERE ID=26078"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($GutsAndGore -or $GutsAndGoreBaseline){
 $probeSource = "$PSScriptRoot/GutsAndGoreNative.cpp"
 [IO.File]::WriteAllText("$out/NativeGutsAndGore.inc",$query+' " WHERE ID=26995"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($SamophlangeManual -or $SamophlangeManualBaseline){
 $probeSource = "$PSScriptRoot/SamophlangeManualNative.cpp"
 [IO.File]::WriteAllText("$out/NativeSamophlangeManual.inc",$query+' " WHERE ID IN (3924,14004,14006) ORDER BY ID"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($SamophlangeManual){
  foreach($file in @('Entities/GameObject/QuestItemObjectUse.h','Entities/Player/Player.cpp','Spells/Spell.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($BloodcupBraid -or $BloodcupBraidBaseline){
 $probeSource = "$PSScriptRoot/BloodcupBraidNative.cpp"
 [IO.File]::WriteAllText("$out/NativeBloodcupBraid.inc",$query+' " WHERE ID IN (13873,13875) ORDER BY ID"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($BloodcupBraid){
  foreach($file in @('Entities/GameObject/QuestItemObjectUse.h','Entities/GameObject/GameObject.cpp','Entities/Player/Player.cpp','Spells/Spell.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($PrincessStillpine -or $PrincessStillpineBaseline){
 $probeSource = "$PSScriptRoot/PrincessStillpineNative.cpp"
 [IO.File]::WriteAllText("$out/NativePrincessStillpine.inc",$query+' " WHERE ID IN (9538,9667) ORDER BY ID"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($PrincessStillpine){
  foreach($file in @('Entities/GameObject/QuestItemObjectUse.h','Entities/GameObject/GameObject.cpp','Handlers/SpellHandler.cpp','Spells/Spell.cpp','Spells/SpellEffects.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
  if((Get-Item "$repo/src/server/scripts/Kalimdor/zone_bloodmyst_isle.cpp").LastWriteTimeUtc -gt (Get-Item $scriptsLibrary).LastWriteTimeUtc){
   throw 'Actual scripts library predates the Bloodmyst princess scripts'
  }
 }
}
if($AkidaCages -or $AkidaCagesBaseline){
 $probeSource = "$PSScriptRoot/AkidaCagesNative.cpp"
 [IO.File]::WriteAllText("$out/NativeAkidaCages.inc",$query+' " WHERE ID=9544"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($AkidaCages){
  foreach($file in @('Entities/GameObject/QuestItemObjectUse.h','Entities/GameObject/GameObject.cpp','Handlers/SpellHandler.cpp','Spells/Spell.cpp','Spells/SpellEffects.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
  if((Get-Item "$repo/src/server/scripts/Kalimdor/zone_azuremyst_isle.cpp").LastWriteTimeUtc -gt (Get-Item $scriptsLibrary).LastWriteTimeUtc){
   throw 'Actual scripts library predates the Bristlelimb cage scripts'
  }
 }
}
if($RavagerCage -or $RavagerCageBaseline){
 $probeSource = "$PSScriptRoot/RavagerCageNative.cpp"
 [IO.File]::WriteAllText("$out/NativeRavagerCage.inc",$query+' " WHERE ID=9582"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($RavagerCage){
  foreach($file in @('Entities/GameObject/QuestItemObjectUse.h','Entities/GameObject/GameObject.cpp','Handlers/SpellHandler.cpp','Spells/Spell.cpp','Spells/SpellEffects.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
  if((Get-Item "$repo/src/server/scripts/Kalimdor/zone_azuremyst_isle.cpp").LastWriteTimeUtc -gt (Get-Item $scriptsLibrary).LastWriteTimeUtc){
   throw 'Actual scripts library predates the Azuremyst cage scripts'
  }
 }
}
if($AmaniCatacombs -or $AmaniCatacombsBaseline){
 $probeSource = "$PSScriptRoot/AmaniCatacombsNative.cpp"
 [IO.File]::WriteAllText("$out/NativeAmaniCatacombs.inc",$query+' " WHERE ID=9193"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($AmaniCatacombs){
  foreach($file in @('Entities/GameObject/QuestItemObjectUse.h','Entities/GameObject/GameObject.cpp','Entities/Player/Player.cpp','Handlers/MiscHandler.cpp','Handlers/SpellHandler.cpp','Spells/Spell.cpp','Spells/SpellEffects.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($StagnantOasis -or $StagnantOasisBaseline){
 $probeSource = "$PSScriptRoot/StagnantOasisNative.cpp"
 [IO.File]::WriteAllText("$out/NativeStagnantOasis.inc",$query+' " WHERE ID=877"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($StagnantOasis){
  foreach($file in @('Entities/GameObject/QuestItemObjectUse.h','Entities/GameObject/GameObject.cpp','Entities/GameObject/GameObject.h','Handlers/SpellHandler.cpp','Spells/Spell.cpp','Spells/SpellEffects.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($Dozercism -or $DozercismBaseline){
 $probeSource = "$PSScriptRoot/DozercismNative.cpp"
 [IO.File]::WriteAllText("$out/NativeDozercism.inc",$query+' " WHERE ID IN (14209,14423) ORDER BY ID"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($Dozercism){
  foreach($file in @('AI/SmartScripts/Dozercism.h','AI/SmartScripts/SmartAI.cpp','AI/SmartScripts/SmartScript.cpp','Entities/Creature/Creature.cpp','Entities/Creature/TemporarySummon.cpp','Entities/Player/KillRewarder.cpp','Entities/Player/Player.cpp','Maps/Map.cpp','Scripting/MapScripts.cpp','Spells/Spell.cpp','Spells/SpellEffects.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($SfgMinefield -or $SfgMinefieldBaseline){
 $probeSource = "$PSScriptRoot/SfgMinefieldNative.cpp"
 [IO.File]::WriteAllText("$out/NativeSfgMinefield.inc",$query+' " WHERE ID=14472"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($SfgMinefield){
  foreach($file in @('Entities/Player/Player.cpp','Entities/Player/Player.h','Entities/Player/KillRewarder.cpp','Entities/Player/KillRewarder.h','Entities/Unit/Unit.cpp','Spells/Spell.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($Absorbent -or $AbsorbentBaseline){
 $probeSource = "$PSScriptRoot/AbsorbentNative.cpp"
 [IO.File]::WriteAllText("$out/NativeAbsorbent.inc",$query+' " WHERE ID=14323"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($Absorbent){
  foreach($file in @('AI/SmartScripts/SmartScript.cpp','Spells/Spell.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($VileFinLeash -or $VileFinLeashBaseline){
 $probeSource = "$PSScriptRoot/VileFinLeashNative.cpp"
 [IO.File]::WriteAllText("$out/NativeVileFinLeash.inc",$query+' " WHERE ID=24974"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($VileFinLeash){
  foreach($file in @('AI/SmartScripts/SmartScript.cpp','Maps/Map.cpp','Spells/Spell.cpp','Entities/Creature/TemporarySummon.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($BalboaBasalt -or $BalboaBasaltBaseline){
 $probeSource = "$PSScriptRoot/BalboaBasaltNative.cpp"
 [IO.File]::WriteAllText("$out/NativeBalboaBasalt.inc",$query+' " WHERE ID=14250"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($BalboaBasalt){
  foreach($file in @('AI/SmartScripts/SmartScript.cpp','Maps/Map.cpp','Spells/Spell.cpp','Entities/Object/Object.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($TongaGuide -or $TongaGuideBaseline){
 $probeSource = "$PSScriptRoot/TongaGuideNative.cpp"
 [IO.File]::WriteAllText("$out/NativeTongaGuide.inc",$query+' " WHERE ID=13988"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($TongaGuide){
  foreach($file in @('AI/SmartScripts/SmartScript.cpp','Maps/Map.cpp','Spells/Spell.cpp','Spells/SpellEffects.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($HogtiedRazormane -or $HogtiedRazormaneBaseline){
 $probeSource = "$PSScriptRoot/HogtiedRazormaneNative.cpp"
 [IO.File]::WriteAllText("$out/NativeHogtiedRazormane.inc",$query+' " WHERE ID=13961"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($HogtiedRazormane){
  foreach($file in @('AI/SmartScripts/SmartScript.cpp','Maps/Map.cpp','Entities/Creature/TemporarySummon.cpp','Spells/Spell.cpp','Spells/Spell.h')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($MonkeyBusiness -or $MonkeyBusinessBaseline -or $MonkeyBusinessFlightBaseline){
 $probeSource = "$PSScriptRoot/MonkeyBusinessNative.cpp"
 [IO.File]::WriteAllText("$out/NativeMonkeyBusiness.inc",$query+' " WHERE ID=14019"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($MonkeyBusiness){
  foreach($file in @('AI/SmartScripts/SmartScript.cpp','Spells/Spell.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($StillpinePrimer -or $StillpinePrimerBaseline){
 $probeSource = "$PSScriptRoot/StillpinePrimerNative.cpp"
 [IO.File]::WriteAllText("$out/NativeStillpinePrimer.inc",$query+' " WHERE ID IN(9538,9539)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($StillpinePrimer -and (Get-Item "$repo/src/server/game/Spells/SpellEffects.cpp").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw 'Actual game library predates the native study event fix'}
}
$extra=@('src/server/scripts/Legion','src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include')|ForEach-Object {Join-Path $repo $_}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$language=switch($group.ClCompile.LanguageStandard){'stdcpp14'{'/std:c++14'}'stdcpp17'{'/std:c++17'}'stdcpp20'{'/std:c++20'}'stdcpplatest'{'/std:c++latest'}default{throw 'Unsupported native language'}}
if($group.ClCompile.RuntimeLibrary -ne 'MultiThreadedDLL'){throw 'Expected actual Release /MD ABI'}
$compilerArgs=@('/nologo','/EHsc',$language,'/MD','/DNDEBUG','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
if($Baseline){$compilerArgs+='/DQUEST_OBJECT_LOOT_BASELINE'}
if($SeasoningBaseline){$compilerArgs+='/DSEASONING_UNGUARDED_BASELINE'}
if($AquantionBaseline){$compilerArgs+='/DAQUANTION_UNGUARDED_BASELINE'}
if($AnowynBaseline){$compilerArgs+='/DANOWYN_UNGUARDED_BASELINE'}
if($AlliedCityBaseline){$compilerArgs+='/DALLIED_CITY_BASELINE'}
if($DuskwitherBaseline){$compilerArgs+='/DDUSKWITHER_BASELINE'}
if($RedSnapperBaseline){$compilerArgs+='/DRED_SNAPPER_BASELINE'}
if($SwiftDisciplineBaseline){$compilerArgs+='/DSWIFT_DISCIPLINE_BASELINE'}
if($ExtinguishingHopeBaseline){$compilerArgs+='/DEXTINGUISHING_HOPE_BASELINE'}
if($MulgoreCagesBaseline){$compilerArgs+='/DMULGORE_CAGES_BASELINE'}
if($AshenvaleFelWoodBaseline){$compilerArgs+='/DASHENVALE_FEL_WOOD_BASELINE'}
if($ElwynnQuestsBaseline){$compilerArgs+='/DELWYNN_QUESTS_BASELINE'}
if($SupplyChestsBaseline){$compilerArgs+='/DSUPPLY_CHESTS_BASELINE'}
if($StillpinePrimerBaseline){$compilerArgs+='/DSTILLPINE_PRIMER_BASELINE'}
if($MonkeyBusinessBaseline){$compilerArgs+='/DMONKEY_BUSINESS_BASELINE'}
if($MonkeyBusinessFlightBaseline){$compilerArgs+='/DMONKEY_BUSINESS_FLIGHT_BASELINE'}
if($HogtiedRazormaneBaseline){$compilerArgs+='/DHOGTIED_RAZORMANE_BASELINE'}
if($TongaGuideBaseline){$compilerArgs+='/DTONGA_GUIDE_BASELINE'}
if($AirfieldFirefightingBaseline){$compilerArgs+='/DAIRFIELD_FIREFIGHTING_BASELINE'}
if($FelfireSeedlingsBaseline){$compilerArgs+='/DFELFIRE_SEEDLINGS_BASELINE'}
if($BattleboarTroughsBaseline){$compilerArgs+='/DBATTLEBOAR_TROUGHS_BASELINE'}
if($RavagerCageBaseline){$compilerArgs+='/DRAVAGER_CAGE_BASELINE'}
if($AkidaCagesBaseline){$compilerArgs+='/DAKIDA_CAGES_BASELINE'}
if($PrincessStillpineBaseline){$compilerArgs+='/DPRINCESS_STILLPINE_BASELINE'}
if($BloodcupBraidBaseline){$compilerArgs+='/DBLOODCUP_BRAID_BASELINE'}
if($SamophlangeManualBaseline){$compilerArgs+='/DSAMOPHLANGE_MANUAL_BASELINE'}
if($GutsAndGoreBaseline){$compilerArgs+='/DGUTS_AND_GORE_BASELINE'}
if($AmaniCatacombsBaseline){$compilerArgs+='/DAMANI_CATACOMBS_BASELINE'}
if($StagnantOasisBaseline){$compilerArgs+='/DSTAGNANT_OASIS_BASELINE'}
if($DozercismBaseline){$compilerArgs+='/DDOZERCISM_BASELINE'}
if($SfgMinefieldBaseline){$compilerArgs+='/DSFG_MINEFIELD_BASELINE'}
if($AbsorbentBaseline){$compilerArgs+='/DABSORBENT_BASELINE'}
if($VileFinLeashBaseline){$compilerArgs+='/DVILE_FIN_LEASH_BASELINE'}
if($BalboaBasaltBaseline){$compilerArgs+='/DBALBOA_BASALT_BASELINE'}
if($ForestHeartBaseline){$compilerArgs+='/DFOREST_HEART_BASELINE'}
$compilerArgs+=($includes+$extra+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include',$out)|ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$probeSource+'"'
$compilerArgs+=@('/Fe"'+$out+'/probe.exe"','/Fo"'+$out+'/probe.obj"','/link','/DEBUG','/MAP:"'+$out+'/probe.map"','/PDB:"'+$out+'/probe.pdb"')
$compilerArgs+=($libs|ForEach-Object {'"'+$_+'"'})
[IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n")
& "$out/build.cmd";if($LASTEXITCODE){throw 'Native quest-object loot build failed'}
$runtime="$repo/build-extractors/bin/Release"
$run="$out/run-$(Get-Date -Format yyyyMMdd-HHmmss)";New-Item -ItemType Directory -Force $run|Out-Null
$config=Get-Content -LiteralPath $(if($FixtureConfig){$FixtureConfig}else{"$runtime/worldserver.conf"}) -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$run.Replace('\','/')+'"'))
[IO.File]::WriteAllText("$run/worldserver.conf",$config)
$prior=$env:PATH
try{
 $env:PATH="$runtime;$prior";Push-Location $runtime
 try{& "$out/probe.exe" "$run/worldserver.conf" "$runtime/" > "$run/stdout.log" 2> "$run/stderr.log";$exit=$LASTEXITCODE}finally{Pop-Location}
}finally{$env:PATH=$prior}
Get-Content "$run/stdout.log"|Where-Object {$_ -match '^(PASS|EXPECTED_BROKEN|CASE)'}
$expectedAmaniDiagnostic = $AmaniCatacombsBaseline -and
    [IO.File]::ReadAllText("$run/stderr.log").Trim() -ceq 'ERROR [entities.player] Deprecated function AreaExploredOrEventHappens called for quest 9193'
if($exit -or ((Get-Item "$run/stderr.log").Length -and !$expectedAmaniDiagnostic)){Get-Content "$run/stderr.log" -Tail 12;throw "Native quest-object loot exit=$exit or stderr"}
if($expectedAmaniDiagnostic){Get-Content "$run/stderr.log"}
if((Test-Path "$run/DBErrors.log") -and (Get-Item "$run/DBErrors.log").Length){throw 'Native loot emitted DBErrors; inspect preserved evidence'}
@{baseline=[bool]($Baseline -or $SeasoningBaseline -or $AquantionBaseline -or $AnowynBaseline -or $AlliedCityBaseline -or $DuskwitherBaseline -or $RedSnapperBaseline -or $SwiftDisciplineBaseline -or $ExtinguishingHopeBaseline -or $MulgoreCagesBaseline -or $AshenvaleFelWoodBaseline -or $ElwynnQuestsBaseline -or $SupplyChestsBaseline -or $ForestHeartBaseline -or $StillpinePrimerBaseline -or $MonkeyBusinessBaseline -or $MonkeyBusinessFlightBaseline -or $HogtiedRazormaneBaseline -or $TongaGuideBaseline -or $AirfieldFirefightingBaseline -or $FelfireSeedlingsBaseline -or $BattleboarTroughsBaseline -or $BalboaBasaltBaseline -or $VileFinLeashBaseline -or $AbsorbentBaseline -or $SfgMinefieldBaseline -or $DozercismBaseline -or $StagnantOasisBaseline -or $AmaniCatacombsBaseline -or $RavagerCageBaseline -or $AkidaCagesBaseline -or $PrincessStillpineBaseline -or $BloodcupBraidBaseline -or $SamophlangeManualBaseline -or $GutsAndGoreBaseline);game=(Get-FileHash $game).Hash;scripts=(Get-FileHash $scriptsLibrary).Hash;executable=(Get-FileHash "$out/probe.exe").Hash;source=(Get-FileHash $probeSource).Hash;run=$run;exit=$exit}|ConvertTo-Json|Set-Content "$out/native-inputs.json"
'PASS native execution/exit/stdout/stderr/DBErrors; wrapper must also inspect crash and appended server diagnostics'
