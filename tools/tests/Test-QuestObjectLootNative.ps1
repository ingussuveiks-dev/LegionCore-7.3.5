param([switch]$Baseline,[string]$OutputDirectory='',[string]$BaselineDirectory='',[switch]$PassingWisdomBaseline,[switch]$PassingWisdomOrphanBaseline,[switch]$WanderingIsleBalloon,[switch]$WanderingIsleBalloonBaseline,[switch]$WanderingIsleMandori,[switch]$WanderingIsleMandoriBaseline,[switch]$WanderingIsleSouth,[switch]$WanderingIsleSouthBaseline,[switch]$WanderingIsleMedicalBaseline,[switch]$WanderingIsleRescue,[switch]$WanderingIsleRescueBaseline,[switch]$WanderingIsleThorn,[switch]$WanderingIsleThornBaseline,[switch]$WanderingIsleHealing,[switch]$WanderingIsleHealingBaseline,[switch]$WanderingIsleDeparture,[switch]$WanderingIsleDepartureBaseline,
      [switch]$MoonkinQuestPOI,[switch]$MoonkinMonitoring,[switch]$CameraQuestPOI,[switch]$BertsBots,[switch]$LegionAnniversary,[switch]$OnTheMend,[switch]$DreadlordsPrizePortal,[switch]$DreadlordsPrizePortalBaseline,[switch]$SunGate,[switch]$SunGateBaseline,
      [switch]$AzsharaDetonator,[switch]$AzsharaDetonatorBaseline,[switch]$LowLevelWorldSpawns,[switch]$GilneasFlank,[switch]$WoodsEndCoffer,[switch]$WoodsEndCofferBaseline,[switch]$FromTheShadows,[switch]$FromTheShadowsBaseline,
      [switch]$Volcanoth,[switch]$VolcanothBaseline,[switch]$OldFriends,[switch]$ScorpidVenom,[switch]$ScorpidVenomBaseline,[switch]$PricelessTreasures,[switch]$PricelessTreasuresBaseline,[switch]$WestfallRise,[switch]$WestfallVision,[switch]$WestfallVisionObserve,[switch]$WestfallVisionScene,[switch]$WestfallStew,[switch]$WestfallStewBaseline,[switch]$NoTanks,[switch]$NoTanksBaseline,[switch]$PaintItBlack,[switch]$PaintItBlackBaseline,
      [switch]$DownWithCrushcog,[switch]$DownWithCrushcogBaseline,[switch]$StaggeringEffort,[switch]$StaggeringEffortBaseline,
      [switch]$RedridgeSeaforium,[switch]$RedridgeSeaforiumBaseline,[switch]$DeeprunRat,[switch]$DeeprunRatObserve,[switch]$HighestQuestPOI,[switch]$VisionQuestPOI,[switch]$LegacyQuestPOI,[switch]$OslowQuestPOI,[switch]$EtchedNote,[switch]$ForemanOslow,[switch]$DarkshoreWildlifeFlight,[switch]$BrazieBasicBotany,[switch]$BrazieFlowerPower,[switch]$BrazieGhouls,[switch]$BraziePumpkin,[switch]$BrazieWarden,[switch]$BrazieGarden,
      [switch]$BlackrockPrisoners,[switch]$BlackrockPrisonersBaseline,[switch]$WanderingFirstLesson,
      [switch]$IlgalarWard,[switch]$IlgalarWardBaseline,
      [switch]$ChallengerFires,[switch]$ChallengerFiresBaseline,
      [switch]$HuoOfferings,[switch]$HuoOfferingsBaseline,[switch]$SingingPools,[switch]$SingingPoolsBaseline,[switch]$SingingPoolsWaterBaseline,
      [switch]$SingingPoolsReedsBaseline,[switch]$SingingPoolsRockBaseline,[switch]$SingingPoolsPearlBaseline,[switch]$SingingPoolsShuAdmissionBaseline,[switch]$SingingPoolsReflectionBaseline,[switch]$SingingPoolsCoaxBaseline,[switch]$SingingPoolsSpoutBaseline,[switch]$SingingPoolsCartBaseline,[switch]$SingingPoolsFarmBaseline,[switch]$SingingPoolsMalletBaseline,[switch]$SingingPoolsGongBaseline,[switch]$SingingPoolsWugouBaseline,[switch]$SingingPoolsScrollBaseline,[switch]$SingingPoolsBreezeSuppliesBaseline,[switch]$SingingPoolsFeFengBaseline,[switch]$SingingPoolsRuffianBaseline,[switch]$SingingPoolsWindEscortBaseline,[switch]$SingingPoolsZhaoBaseline,
      [switch]$ForestEttin,[switch]$ForestEttinBaseline,
      [switch]$KezanHotRod,[switch]$KezanHotRodBaseline,
      [switch]$TakingPrecautions,[switch]$CandyBuckets,[switch]$Collector,[switch]$Scenes,[switch]$Kyle,[switch]$Seasoning,[switch]$SeasoningBaseline,
      [switch]$Aquantion,[switch]$AquantionBaseline,[switch]$Anowyn,[switch]$AnowynBaseline,
      [switch]$AlliedCity,[switch]$AlliedCityBaseline,[switch]$Duskwither,[switch]$DuskwitherBaseline,
      [switch]$RedSnapper,[switch]$RedSnapperBaseline,[switch]$SwiftDiscipline,[switch]$SwiftDisciplineBaseline,
      [switch]$ExtinguishingHope,[switch]$ExtinguishingHopeBaseline,
      [switch]$MulgoreCages,[switch]$MulgoreCagesBaseline,
      [switch]$AshenvaleFelWood,[switch]$AshenvaleFelWoodBaseline,
      [switch]$ElwynnQuests,[switch]$ElwynnQuestsBaseline,
      [switch]$SupplyChests,[switch]$SupplyChestsBaseline,
      [switch]$ForestHeart,[switch]$ForestHeartBaseline,[switch]$DarkmoonStrength,[switch]$Tortusk,[switch]$ClusterCluck,[switch]$PoolPony,[switch]$MinerTroubles,[switch]$DreadHead,[switch]$CaravanDelivery,[switch]$WebbedResearcher,[switch]$LiFeiDuel,[switch]$ShangXiEscort,[switch]$YurivPendant,[switch]$FelfireCanister,[switch]$LifebringerSapling,[switch]$LordamereCannon,[switch]$TitansTerminal,[switch]$StillpinePrimer,[switch]$StillpinePrimerBaseline,[switch]$MonkeyBusiness,[switch]$MonkeyBusinessBaseline,[switch]$MonkeyBusinessFlightBaseline,[switch]$HogtiedRazormane,[switch]$HogtiedRazormaneBaseline,[switch]$TongaGuide,[switch]$TongaGuideBaseline,[switch]$AirfieldFirefighting,[switch]$AirfieldFirefightingBaseline,[switch]$FelfireSeedlings,[switch]$FelfireSeedlingsBaseline,[switch]$BattleboarTroughs,[switch]$BattleboarTroughsBaseline,[switch]$BalboaBasalt,[switch]$BalboaBasaltBaseline,[switch]$VileFinLeash,[switch]$VileFinLeashBaseline,[switch]$Absorbent,[switch]$AbsorbentBaseline,[switch]$SfgMinefield,[switch]$SfgMinefieldBaseline,[switch]$Dozercism,[switch]$DozercismBaseline,[switch]$StagnantOasis,[switch]$StagnantOasisBaseline,[switch]$AmaniCatacombs,[switch]$AmaniCatacombsBaseline,[switch]$RavagerCage,[switch]$RavagerCageBaseline,[switch]$AkidaCages,[switch]$AkidaCagesBaseline,[switch]$PrincessStillpine,[switch]$PrincessStillpineBaseline,[switch]$BloodcupBraid,[switch]$BloodcupBraidBaseline,[switch]$SamophlangeManual,[switch]$SamophlangeManualBaseline,[switch]$GutsAndGore,[switch]$GutsAndGoreBaseline,[switch]$ThelgenSeismograph,[switch]$ThelgenSeismographBaseline,[switch]$LovelyCharmGifts,[switch]$LovelyCharmGiftsBaseline,[switch]$DarkmoonCrafting,[switch]$DarkmoonCraftingBaseline,[switch]$DarkmoonFairTargets,[switch]$DarkmoonFairTargetsBaseline,[switch]$DarkmoonBanner,[switch]$DarkmoonBannerBaseline,[switch]$IronforgeBait,[switch]$IronforgeBaitBaseline,[switch]$IronforgeRations,[switch]$IronforgeRationsBaseline,[switch]$LiberationDay,[switch]$LiberationDayBaseline,[switch]$IncendiciteOre,[switch]$IncendiciteOreBaseline,[switch]$KneeDeep,[switch]$KneeDeepBaseline,[switch]$PerfumedGuardAnalysis,[switch]$PerfumedGuardAnalysisBaseline,[switch]$SwiftclawRide,[switch]$SwiftclawRideBaseline,[switch]$KorothBanner,[switch]$KorothBannerBaseline,[string]$FixtureConfig='')
$ErrorActionPreference='Stop'
if(($PassingWisdomBaseline -or $PassingWisdomOrphanBaseline -or $WanderingIsleBalloon -or $WanderingIsleMandori) -and !$ShangXiEscort){throw 'Wandering Isle continuation probes require ShangXiEscort'}
if($WanderingIsleBalloonBaseline -and !$WanderingIsleBalloon){throw 'WanderingIsleBalloonBaseline requires WanderingIsleBalloon'}
if($WanderingIsleMandoriBaseline -and !$WanderingIsleMandori){throw 'WanderingIsleMandoriBaseline requires WanderingIsleMandori'}
if(($WanderingIsleSouthBaseline -or $WanderingIsleMedicalBaseline) -and !$WanderingIsleSouth){throw 'Wandering Isle supply baselines require WanderingIsleSouth'}
if(($WanderingIsleRescue -or $WanderingIsleRescueBaseline) -and !$WanderingIsleSouth){throw 'Wandering Isle rescue probes require WanderingIsleSouth'}
if(($WanderingIsleThorn -or $WanderingIsleThornBaseline) -and !$WanderingIsleSouth){throw 'Wandering Isle thorn probes require WanderingIsleSouth'}
if(($WanderingIsleHealing -or $WanderingIsleHealingBaseline) -and !$WanderingIsleThorn){throw 'Wandering Isle healing probes require WanderingIsleThorn'}
if($WanderingIsleDeparture -and !$WanderingIsleHealing){throw 'Wandering Isle departure probe requires WanderingIsleHealing'}
if($WanderingIsleDepartureBaseline -and !$WanderingIsleDeparture){throw 'Wandering Isle departure baseline requires WanderingIsleDeparture'}
if($WestfallVisionObserve -and !$WestfallVision){throw 'The Westfall observation requires the WestfallVision probe'}
if($WestfallVisionScene -and (!$WestfallVision -or $WestfallVisionObserve)){throw 'The repaired Vision scene requires WestfallVision with the migrated world data'}
if($DeeprunRatObserve -and !$DeeprunRat){throw 'DeeprunRatObserve requires DeeprunRat and the pre-migration database'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$scopedProbes = @(@($TakingPrecautions,$Collector,$Scenes,$Kyle,$Seasoning,$SeasoningBaseline,$Aquantion,$AquantionBaseline,$Anowyn,$AnowynBaseline,$AlliedCity,$AlliedCityBaseline,$Duskwither,$DuskwitherBaseline,$RedSnapper,$RedSnapperBaseline,$SwiftDiscipline,$SwiftDisciplineBaseline,$ExtinguishingHope,$ExtinguishingHopeBaseline,$MulgoreCages,$MulgoreCagesBaseline,$AshenvaleFelWood,$AshenvaleFelWoodBaseline,$ElwynnQuests,$ElwynnQuestsBaseline,$SupplyChests,$SupplyChestsBaseline,$ForestHeart,$ForestHeartBaseline,$DarkmoonStrength,$Tortusk,$ClusterCluck,$PoolPony,$MinerTroubles,$DreadHead,$CaravanDelivery,$WebbedResearcher,$LiFeiDuel,$ShangXiEscort,$YurivPendant,$FelfireCanister,$LifebringerSapling,$LordamereCannon,$TitansTerminal,$StillpinePrimer,$StillpinePrimerBaseline,$MonkeyBusiness,$MonkeyBusinessBaseline,$MonkeyBusinessFlightBaseline,$HogtiedRazormane,$HogtiedRazormaneBaseline,$TongaGuide,$TongaGuideBaseline,$AirfieldFirefighting,$AirfieldFirefightingBaseline,$FelfireSeedlings,$FelfireSeedlingsBaseline,$BattleboarTroughs,$BattleboarTroughsBaseline,$BalboaBasalt,$BalboaBasaltBaseline,$VileFinLeash,$VileFinLeashBaseline,$Absorbent,$AbsorbentBaseline,$SfgMinefield,$SfgMinefieldBaseline,$Dozercism,$DozercismBaseline,$StagnantOasis,$StagnantOasisBaseline,$AmaniCatacombs,$AmaniCatacombsBaseline,$RavagerCage,$RavagerCageBaseline,$AkidaCages,$AkidaCagesBaseline,$PrincessStillpine,$PrincessStillpineBaseline,$BloodcupBraid,$BloodcupBraidBaseline,$SamophlangeManual,$SamophlangeManualBaseline,$GutsAndGore,$GutsAndGoreBaseline,$ThelgenSeismograph,$ThelgenSeismographBaseline,$LovelyCharmGifts,$LovelyCharmGiftsBaseline,$DarkmoonCrafting,$DarkmoonCraftingBaseline,$DarkmoonFairTargets,$DarkmoonFairTargetsBaseline,$DarkmoonBanner,$DarkmoonBannerBaseline,$IronforgeBait,$IronforgeBaitBaseline,$IronforgeRations,$IronforgeRationsBaseline,$LiberationDay,$LiberationDayBaseline,$IncendiciteOre,$IncendiciteOreBaseline,$KneeDeep,$KneeDeepBaseline,$PerfumedGuardAnalysis,$PerfumedGuardAnalysisBaseline,$SwiftclawRide,$SwiftclawRideBaseline,$KorothBanner,$KorothBannerBaseline,$WoodsEndCoffer,$WoodsEndCofferBaseline,$FromTheShadows,$FromTheShadowsBaseline,$Volcanoth,$VolcanothBaseline,$OldFriends,$ScorpidVenom,$ScorpidVenomBaseline,$PricelessTreasures,$PricelessTreasuresBaseline,$WestfallRise,$WestfallVision,$WestfallStew,$WestfallStewBaseline,$NoTanks,$NoTanksBaseline,$PaintItBlack,$PaintItBlackBaseline,$DownWithCrushcog,$DownWithCrushcogBaseline,$StaggeringEffort,$StaggeringEffortBaseline,$RedridgeSeaforium,$RedridgeSeaforiumBaseline,$BlackrockPrisoners,$BlackrockPrisonersBaseline,$IlgalarWard,$IlgalarWardBaseline,$ChallengerFires,$ChallengerFiresBaseline,$HuoOfferings,$HuoOfferingsBaseline,$ForestEttin,$ForestEttinBaseline,$KezanHotRod,$KezanHotRodBaseline) | Where-Object { $_ }).Count
if($DeeprunRat){++$scopedProbes}
if($HighestQuestPOI -or $VisionQuestPOI -or $LegacyQuestPOI -or $OslowQuestPOI -or $MoonkinQuestPOI -or $CameraQuestPOI){++$scopedProbes}
if($EtchedNote){++$scopedProbes}
if($ForemanOslow){++$scopedProbes}
if($CandyBuckets){++$scopedProbes}
if($MoonkinMonitoring){++$scopedProbes}
if($BertsBots){++$scopedProbes}
if($LegionAnniversary){++$scopedProbes}
if($OnTheMend){++$scopedProbes}
if($DreadlordsPrizePortal -or $DreadlordsPrizePortalBaseline){++$scopedProbes}
if($DreadlordsPrizePortal -and $DreadlordsPrizePortalBaseline){throw 'Select either the working portal or its pre-migration fixture'}
if($SunGate -or $SunGateBaseline){++$scopedProbes}
if($SunGate -and $SunGateBaseline){throw 'Select either the working Sun Gate or its pre-migration fixture'}
if($AzsharaDetonator -or $AzsharaDetonatorBaseline){++$scopedProbes}
if($AzsharaDetonator -and $AzsharaDetonatorBaseline){throw 'Select either the working detonator or its pre-migration fixture'}
if($LowLevelWorldSpawns){++$scopedProbes}
if($GilneasFlank){++$scopedProbes}
if($DarkshoreWildlifeFlight){++$scopedProbes}
if($WanderingIsleSouth){++$scopedProbes}
if($BrazieBasicBotany){++$scopedProbes}
if($BrazieFlowerPower){++$scopedProbes}
if($BrazieGhouls){++$scopedProbes}
if($BraziePumpkin){++$scopedProbes}
if($BrazieWarden){++$scopedProbes}
if($BrazieGarden){++$scopedProbes}
if($WanderingFirstLesson){++$scopedProbes}
$singingPoolsProbe=$SingingPools -or $SingingPoolsBaseline -or $SingingPoolsWaterBaseline -or $SingingPoolsReedsBaseline -or $SingingPoolsRockBaseline -or $SingingPoolsPearlBaseline -or $SingingPoolsShuAdmissionBaseline -or $SingingPoolsReflectionBaseline -or $SingingPoolsCoaxBaseline -or $SingingPoolsSpoutBaseline -or $SingingPoolsCartBaseline -or $SingingPoolsFarmBaseline -or $SingingPoolsMalletBaseline -or $SingingPoolsGongBaseline -or $SingingPoolsWugouBaseline -or $SingingPoolsScrollBaseline -or $SingingPoolsBreezeSuppliesBaseline -or $SingingPoolsFeFengBaseline -or $SingingPoolsRuffianBaseline -or $SingingPoolsWindEscortBaseline -or $SingingPoolsZhaoBaseline
if($singingPoolsProbe){++$scopedProbes}
if($scopedProbes -and ($Baseline -or !$FixtureConfig -or $scopedProbes -gt 1)){throw 'Use one named probe and its private migrated fixture'}
if(!$OutputDirectory -and $BrazieBasicBotany){$OutputDirectory="$repo/build-extractors/runtime-test/quest-brazie-basic-botany"}
if(!$OutputDirectory -and $BrazieFlowerPower){$OutputDirectory="$repo/build-extractors/runtime-test/quest-brazie-flower-power"}
if(!$OutputDirectory -and $BrazieGhouls){$OutputDirectory="$repo/build-extractors/runtime-test/quest-brazie-ghouls"}
if(!$OutputDirectory -and $BraziePumpkin){$OutputDirectory="$repo/build-extractors/runtime-test/quest-brazie-pumpkin"}
if(!$OutputDirectory -and $BrazieWarden){$OutputDirectory="$repo/build-extractors/runtime-test/quest-brazie-warden"}
if(!$OutputDirectory -and $BrazieGarden){$OutputDirectory="$repo/build-extractors/runtime-test/quest-brazie-garden"}
if(!$OutputDirectory -and $DarkshoreWildlifeFlight){$OutputDirectory="$repo/build-extractors/runtime-test/quest-darkshore-wildlife-flight"}
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
if(!$OutputDirectory -and ($NoTanks -or $NoTanksBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-no-tanks"}
if(!$OutputDirectory -and ($KezanHotRod -or $KezanHotRodBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-kezan-hot-rod"}
if(!$OutputDirectory -and ($ForestEttin -or $ForestEttinBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-forest-ettin"}
if(!$OutputDirectory -and ($HuoOfferings -or $HuoOfferingsBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-huo-offerings"}
if(!$OutputDirectory -and $singingPoolsProbe){$OutputDirectory="$repo/build-extractors/runtime-test/quest-singing-pools"}
if(!$OutputDirectory -and ($ChallengerFires -or $ChallengerFiresBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-challenger-fires"}
if(!$OutputDirectory -and ($IlgalarWard -or $IlgalarWardBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-ilgalar-ward"}
if(!$OutputDirectory -and ($BlackrockPrisoners -or $BlackrockPrisonersBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-blackrock-prisoners"}
if(!$OutputDirectory -and ($RedridgeSeaforium -or $RedridgeSeaforiumBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-redridge-seaforium"}
if(!$OutputDirectory -and ($StaggeringEffort -or $StaggeringEffortBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-staggering-effort"}
if(!$OutputDirectory -and ($DownWithCrushcog -or $DownWithCrushcogBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-down-with-crushcog"}
if(!$OutputDirectory -and ($PaintItBlack -or $PaintItBlackBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-paint-it-black"}
if(!$OutputDirectory -and ($WestfallStew -or $WestfallStewBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-westfall-stew"}
if(!$OutputDirectory -and ($PricelessTreasures -or $PricelessTreasuresBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-priceless-treasures"}
if(!$OutputDirectory -and ($ScorpidVenom -or $ScorpidVenomBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-scorpid-venom"}
if(!$OutputDirectory -and ($Volcanoth -or $VolcanothBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-volcanoth"}
if(!$OutputDirectory -and ($FromTheShadows -or $FromTheShadowsBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-from-the-shadows"}
if(!$OutputDirectory -and ($WoodsEndCoffer -or $WoodsEndCofferBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-woods-end-coffer"}
if(!$OutputDirectory -and ($KorothBanner -or $KorothBannerBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-koroth-banner"}
if(!$OutputDirectory -and ($SwiftclawRide -or $SwiftclawRideBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-swiftclaw-ride"}
if(!$OutputDirectory -and ($PerfumedGuardAnalysis -or $PerfumedGuardAnalysisBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-perfumed-guard-analysis"}
if(!$OutputDirectory -and ($KneeDeep -or $KneeDeepBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-knee-deep"}
if(!$OutputDirectory -and ($IncendiciteOre -or $IncendiciteOreBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-incendicite-ore"}
if(!$OutputDirectory -and ($LiberationDay -or $LiberationDayBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-liberation-day"}
if(!$OutputDirectory -and ($IronforgeRations -or $IronforgeRationsBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-ironforge-rations"}
if(!$OutputDirectory -and ($IronforgeBait -or $IronforgeBaitBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-ironforge-bait"}
if(!$OutputDirectory -and ($DarkmoonBanner -or $DarkmoonBannerBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-darkmoon-banner"}
if(!$OutputDirectory -and ($DarkmoonFairTargets -or $DarkmoonFairTargetsBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-darkmoon-fair-targets"}
if(!$OutputDirectory -and ($DarkmoonCrafting -or $DarkmoonCraftingBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-darkmoon-crafting"}
if(!$OutputDirectory -and ($LovelyCharmGifts -or $LovelyCharmGiftsBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-lovely-charm-gifts"}
if(!$OutputDirectory -and ($ThelgenSeismograph -or $ThelgenSeismographBaseline)){$OutputDirectory="$repo/build-extractors/runtime-test/quest-thelgen-seismograph"}
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
}elseif($KezanHotRodBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/kezan-hot-rod-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '9bd5ecdce' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game -or
    (Get-FileHash "$BaselineDirectory/scripts.lib").Hash -ne $manifest.scripts){throw 'Pinned Hot Rod counterfactual libraries differ'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}elseif($_ -eq $scriptsLibrary){"$BaselineDirectory/scripts.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
 $scriptsLibrary="$BaselineDirectory/scripts.lib"
}elseif($ForestEttinBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/forest-ettin-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne 'ebd9b39a7' -or (Get-FileHash "$BaselineDirectory/scripts.lib").Hash -ne $manifest.scripts){throw 'Pinned Ettin counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $scriptsLibrary){"$BaselineDirectory/scripts.lib"}else{$_}}
 $scriptsLibrary="$BaselineDirectory/scripts.lib"
}elseif($HuoOfferingsBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/huo-offerings-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne 'fbf7ccc0a' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game -or
    (Get-FileHash "$BaselineDirectory/scripts.lib").Hash -ne $manifest.scripts){throw 'Pinned Huo counterfactual libraries differ'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}elseif($_ -eq $scriptsLibrary){"$BaselineDirectory/scripts.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
 $scriptsLibrary="$BaselineDirectory/scripts.lib"
}elseif($ChallengerFiresBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/shrine-of-inner-light-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '667b1a942' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned brazier counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($IlgalarWardBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/ilgalar-ward-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '1113b954f' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Ilgalar ward counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($BlackrockPrisonersBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/blackrock-prisoners-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne 'cc1dcdc5b' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Blackrock pen counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($DownWithCrushcogBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/down-with-crushcog-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '1146e5787' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Crushcog counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($PaintItBlackBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/paint-it-black-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '43a83e669' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Paint it Black counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($NoTanksBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/no-tanks-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '824c75357' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned No Tanks counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($WestfallStewBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/westfall-stew-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne 'b8fcdaf69' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game -or
    (Get-FileHash "$BaselineDirectory/scripts.lib").Hash -ne $manifest.scripts){throw 'Pinned Westfall Stew counterfactual libraries differ'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}elseif($_ -eq $scriptsLibrary){"$BaselineDirectory/scripts.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
 $scriptsLibrary="$BaselineDirectory/scripts.lib"
}elseif($PricelessTreasuresBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/priceless-treasures-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '7a4a057d8' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Frozen Artifact counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($ScorpidVenomBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/scorpid-venom-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '25e1da098' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game -or
    (Get-FileHash "$BaselineDirectory/scripts.lib").Hash -ne $manifest.scripts){throw 'Pinned Scorpid Venom counterfactual libraries differ'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}elseif($_ -eq $scriptsLibrary){"$BaselineDirectory/scripts.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
 $scriptsLibrary="$BaselineDirectory/scripts.lib"
}elseif($VolcanothBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/volcanoth-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '32d849522' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Volcanoth counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($FromTheShadowsBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/from-the-shadows-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '773d0223a' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game -or
    (Get-FileHash "$BaselineDirectory/scripts.lib").Hash -ne $manifest.scripts){throw 'Pinned From the Shadows counterfactual libraries differ'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}elseif($_ -eq $scriptsLibrary){"$BaselineDirectory/scripts.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
 $scriptsLibrary="$BaselineDirectory/scripts.lib"
}elseif($WoodsEndCofferBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/woods-end-coffer-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '0469e4254' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Woods End counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($KorothBannerBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/koroths-banner-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne 'ba5c4212c' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Koroth Banner counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($SwiftclawRideBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/young-and-vicious-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne 'cb354092d' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game -or
    (Get-FileHash "$BaselineDirectory/scripts.lib").Hash -ne $manifest.scripts){throw 'Pinned Swiftclaw counterfactual libraries differ'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}elseif($_ -eq $scriptsLibrary){"$BaselineDirectory/scripts.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
 $scriptsLibrary="$BaselineDirectory/scripts.lib"
}elseif($PerfumedGuardAnalysisBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/something-stinks-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '244f8ecdd' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game -or
    (Get-FileHash "$BaselineDirectory/scripts.lib").Hash -ne $manifest.scripts){throw 'Pinned Something Stinks counterfactual libraries differ'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}elseif($_ -eq $scriptsLibrary){"$BaselineDirectory/scripts.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
 $scriptsLibrary="$BaselineDirectory/scripts.lib"
}elseif($KneeDeepBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/knee-deep-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '74692393b' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Knee-Deep counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($IncendiciteOreBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/incendicite-ore-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne 'ed507d0b0' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Incendicite counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($LiberationDayBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/liberation-day-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '361abe91d' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Liberation Day counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($IronforgeRationsBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/ironforge-rations-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '945208bf8' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Ironforge Rations counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($IronforgeBaitBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/ironforge-cooking-fishing-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '7013f8752' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Ironforge Bait counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($DarkmoonBannerBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/darkmoon-banner-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '58a9a30cb' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Darkmoon Banner counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($DarkmoonFairTargetsBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/darkmoon-fair-targets-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '7169bef26' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Darkmoon actor counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($DarkmoonCraftingBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/darkmoon-professions-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne 'e7b264fd7' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Darkmoon crafting counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($LovelyCharmGiftsBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/lovely-charms-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '4176f7331' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Lovely Charm counterfactual library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif($ThelgenSeismographBaseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/chest-quest-loot-markers-20261122/pre-fix"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne 'e8213533b' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned Thelgen counterfactual library differs'}
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
}elseif($OnTheMend -and $BaselineDirectory){
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '41e014f45' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned regional healing counterfactual library differs'}
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
if($CandyBuckets){
 $probeSource = "$PSScriptRoot/HallowsEndCandyBucketsNative.cpp"
 [IO.File]::WriteAllText("$out/NativeHallowsEndCandyBuckets.inc",$query+' " WHERE ID IN(12286,39657,43055)"')
}
if($MoonkinMonitoring){
 $probeSource = "$PSScriptRoot/MoonkinMonitoringNative.cpp"
 [IO.File]::WriteAllText("$out/NativeMoonkinMonitoring.inc",$query+' " WHERE ID=47430"')
}
if($BertsBots){
 $probeSource = "$PSScriptRoot/BertsBotsNative.cpp"
 [IO.File]::WriteAllText("$out/NativeBertsBots.inc",$query+' " WHERE ID=47895"')
}
if($LegionAnniversary){
 $probeSource = "$PSScriptRoot/LegionAnniversaryNative.cpp"
 [IO.File]::WriteAllText("$out/NativeLegionAnniversary.inc",$query+' " WHERE ID IN(43323,43461,43471,43472,47251,47252)"')
}
if($OnTheMend){
 $probeSource = "$PSScriptRoot/OnTheMendNative.cpp"
 [IO.File]::WriteAllText("$out/NativeOnTheMend.inc",$query+' " WHERE ID IN(31308,31309,31548,31549,31551,31552,31553,31554,31556,31568,31571,31572,31573,31574,31576,31577,31579,31580,31582,31583,31585,31586,31587,31588,31589,31591,31592,31785,31821,31822,31823,31824,31825,31826,31827,31828,31830,31831,31832)"')
}
if($DreadlordsPrizePortal -or $DreadlordsPrizePortalBaseline){
 $probeSource = "$PSScriptRoot/DreadlordsPrizePortalNative.cpp"
}
if($SunGate -or $SunGateBaseline){
 $probeSource = "$PSScriptRoot/SunGateNative.cpp"
 [IO.File]::WriteAllText("$out/NativeSunGate.inc",$query+' " WHERE ID IN(9740,9746)"')
}
if($AzsharaDetonator -or $AzsharaDetonatorBaseline){
 $probeSource = "$PSScriptRoot/AzsharaDetonatorNative.cpp"
 [IO.File]::WriteAllText("$out/NativeAzsharaDetonator.inc",$query+' " WHERE ID IN(14475,14476,14477)"')
}
if($LowLevelWorldSpawns){
 $probeSource = "$PSScriptRoot/LowLevelWorldSpawnsNative.cpp"
 [IO.File]::WriteAllText("$out/NativeLowLevelWorldSpawns.inc",$query+' " WHERE ID IN(2498,2499,29401,29412)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($GilneasFlank){
 $probeSource = "$PSScriptRoot/GilneasFlankNative.cpp"
 [IO.File]::WriteAllText("$out/NativeGilneasFlank.inc",$query+' " WHERE ID IN(14386,24575,24592,24672,24677)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
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
if($WanderingFirstLesson){
 $probeSource = "$PSScriptRoot/WanderingFirstLessonNative.cpp"
 [IO.File]::WriteAllText("$out/NativeWanderingLesson.inc",$query+' " WHERE ID BETWEEN 30033 AND 30045 OR ID IN (30027,29406,29408,29409,29410,29414,29417,29418,29419,29424,29522,29524)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($WanderingIsleSouth){
 $probeSource = "$PSScriptRoot/WanderingIsleSouthNative.cpp"
 $questIds = '29792,29793,29794,29795,29796,29797,30589,30590,30591'
 if($WanderingIsleThorn -or $WanderingIsleThornBaseline){$questIds+=',29665,29798,29799,30767'}
 if($WanderingIsleDeparture){$questIds+=',29800,31450'}
 [IO.File]::WriteAllText("$out/NativeWanderingIsleSouth.inc",$query+(' " WHERE ID IN ({0})"' -f $questIds))
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($ShangXiEscort){
 $probeSource = "$PSScriptRoot/ShangXiEscortNative.cpp"
 [IO.File]::WriteAllText("$out/NativeShangXiEscort.inc",$query+' " WHERE ID BETWEEN 29787 AND 29792"')
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
if($NoTanks -or $NoTanksBaseline){
 $probeSource="$PSScriptRoot/NoTanksNative.cpp"
 [IO.File]::WriteAllText("$out/NativeNoTanks.inc",$query+' " WHERE ID IN(26329,26333)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($NoTanks){
  foreach($file in @('Entities/GameObject/QuestItemObjectUse.h','Spells/Spell.cpp','Spells/SpellEffects.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($KezanHotRod -or $KezanHotRodBaseline){
 $probeSource="$PSScriptRoot/KezanHotRodNative.cpp"
 [IO.File]::WriteAllText("$out/NativeKezanHotRod.inc",$query+' " WHERE ID IN(28349,14071)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($ForestEttin -or $ForestEttinBaseline){
 $probeSource="$PSScriptRoot/ForestEttinNative.cpp"
 [IO.File]::WriteAllText("$out/NativeForestEttin.inc",$query+' " WHERE ID IN(27082,27088)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($HuoOfferings -or $HuoOfferingsBaseline){
 $probeSource="$PSScriptRoot/HuoOfferingsNative.cpp"
 [IO.File]::WriteAllText("$out/NativeHuoOfferings.inc",$query+' " WHERE ID IN(29421,29422,29423,29521)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($singingPoolsProbe){
 $probeSource="$PSScriptRoot/SingingPoolsNative.cpp"
 [IO.File]::WriteAllText("$out/NativeSingingPools.inc",$query+' " WHERE ID IN (29423,29521,29661,29662,29663,29665,29666,29676,29677,29678,29679,29680,29768,29769,29770,29771,29772,29774,29775,29776,29777,29778,29779,29780,29781,29782,29783,29784,29785,29786,29787)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($ChallengerFires -or $ChallengerFiresBaseline){
 $probeSource="$PSScriptRoot/ChallengerFiresNative.cpp"
 [IO.File]::WriteAllText("$out/NativeChallengerFires.inc",$query+' " WHERE ID IN(29420,29422,29664)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($IlgalarWard -or $IlgalarWardBaseline){
 $probeSource="$PSScriptRoot/IlgalarWardNative.cpp"
 [IO.File]::WriteAllText("$out/NativeIlgalarWard.inc",$query+' " WHERE ID IN(26693,26694)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($BlackrockPrisoners -or $BlackrockPrisonersBaseline){
 $probeSource="$PSScriptRoot/BlackrockPrisonersNative.cpp"
 [IO.File]::WriteAllText("$out/NativeBlackrockPrisoners.inc",$query+' " WHERE ID IN(26640,26646)"')
 if($BlackrockPrisoners -and (Get-Item "$repo/src/server/game/Entities/GameObject/GameObject.cpp").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw 'The actual game library predates the Blackrock pen'}
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($RedridgeSeaforium -or $RedridgeSeaforiumBaseline){
 $probeSource="$PSScriptRoot/RedridgeSeaforiumNative.cpp"
 [IO.File]::WriteAllText("$out/NativeRedridgeSeaforium.inc",$query+' " WHERE ID IN(26646,26651)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($StaggeringEffort -or $StaggeringEffortBaseline){
 $probeSource="$PSScriptRoot/StaggeringEffortNative.cpp"
 [IO.File]::WriteAllText("$out/NativeStaggeringEffort.inc",$query+' " WHERE ID=26557"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($DownWithCrushcog -or $DownWithCrushcogBaseline){
 $probeSource="$PSScriptRoot/DownWithCrushcogNative.cpp"
 [IO.File]::WriteAllText("$out/NativeDownWithCrushcog.inc",$query+' " WHERE ID IN(26342,26364)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($DownWithCrushcog){
  if((Get-Item "$repo/src/server/game/Maps/Map.cpp").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw 'Actual game library predates orbital marker scope'}
  if((Get-Item "$repo/src/server/scripts/EasternKingdoms/zone_dun_morogh.cpp").LastWriteTimeUtc -gt (Get-Item $scriptsLibrary).LastWriteTimeUtc){throw 'Actual scripts library predates the assault'}
 }
}
if($PaintItBlack -or $PaintItBlackBaseline){
 $probeSource="$PSScriptRoot/PaintItBlackNative.cpp"
 [IO.File]::WriteAllText("$out/NativePaintItBlack.inc",$query+' " WHERE ID IN(26339,26342)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($PaintItBlack){
  foreach($file in @('AI/SmartScripts/SmartScript.cpp','Entities/GameObject/QuestItemObjectUse.h','Spells/Spell.cpp','Spells/SpellEffects.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($WestfallRise){
 $probeSource="$PSScriptRoot/WestfallRiseNative.cpp"
 [IO.File]::WriteAllText("$out/NativeWestfallRise.inc",$query+' " WHERE ID IN (123,26266,26286,26297,26319,26320,26322,26370,26378,26761,28562,39691,40519,42740,42978,42982,44663,46272,46282)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($DeeprunRat){
 $probeSource="$PSScriptRoot/DeeprunRatNative.cpp"
 [IO.File]::WriteAllText("$out/NativeDeeprunRat.inc",$query+' " WHERE ID IN (6661,26322)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($HighestQuestPOI -or $VisionQuestPOI -or $LegacyQuestPOI -or $OslowQuestPOI -or $MoonkinQuestPOI -or $CameraQuestPOI){$probeSource="$PSScriptRoot/QuestMapMarkersNative.cpp"}
if($EtchedNote){
 $probeSource="$PSScriptRoot/EtchedNoteNative.cpp"
 [IO.File]::WriteAllText("$out/NativeEtchedNote.inc",$query+' " WHERE ID IN (24852,3092,27021)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($ForemanOslow){
 $probeSource="$PSScriptRoot/ForemanOslowNative.cpp"
 [IO.File]::WriteAllText("$out/NativeForemanOslow.inc",$query+' " WHERE ID IN (26519,26520)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($WestfallVision){
 $probeSource="$PSScriptRoot/WestfallVisionNative.cpp"
 [IO.File]::WriteAllText("$out/NativeWestfallVision.inc",$query+' " WHERE ID IN (26319,26320,26322)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($WestfallStew -or $WestfallStewBaseline){
 $probeSource="$PSScriptRoot/WestfallStewNative.cpp"
 [IO.File]::WriteAllText("$out/NativeWestfallStew.inc",$query+' " WHERE ID IN(26266,26271,26289,26322)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($PricelessTreasures -or $PricelessTreasuresBaseline){
 $probeSource="$PSScriptRoot/PricelessTreasuresNative.cpp"
 [IO.File]::WriteAllText("$out/NativePricelessTreasures.inc",$query+' " WHERE ID IN(25937)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($PricelessTreasures){
  foreach($file in @('Entities/GameObject/QuestItemObjectUse.h','Entities/GameObject/GameObject.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($ScorpidVenom -or $ScorpidVenomBaseline){
 $probeSource="$PSScriptRoot/ScorpidVenomNative.cpp"
 [IO.File]::WriteAllText("$out/NativeScorpidVenom.inc",$query+' " WHERE ID IN(25165,25170)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
}
if($BrazieBasicBotany -or $BrazieFlowerPower -or $BrazieGhouls -or $BraziePumpkin -or $BrazieWarden -or $BrazieGarden){
 $probeSource="$PSScriptRoot/BrazieBasicBotanyNative.cpp"
 [IO.File]::WriteAllText("$out/NativeBrazieBasicBotany.inc",$query+' " WHERE ID IN (28733,28617,28744,28747,28748,28751)"')
 $brazieStage = if($BrazieGarden){6}elseif($BrazieWarden){5}elseif($BraziePumpkin){4}elseif($BrazieGhouls){3}elseif($BrazieFlowerPower){2}else{1}
 [IO.File]::WriteAllText("$out/NativeBrazieStage.inc","constexpr unsigned BrazieStage = $brazieStage;")
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 foreach($file in @('Entities/Object/Object.cpp','Entities/Unit/Unit.cpp','Maps/Map.cpp')){
  if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
 }
 if((Get-Item "$repo/src/server/scripts/EasternKingdoms/hillsbrad_basic_botany.h").LastWriteTimeUtc -gt (Get-Item $scriptsLibrary).LastWriteTimeUtc){throw 'Actual scripts library predates Brazie'}
}
if($DarkshoreWildlifeFlight){
 $probeSource="$PSScriptRoot/DarkshoreWildlifeFlightNative.cpp"
 [IO.File]::WriteAllText("$out/NativeDarkshoreWildlifeFlight.inc",$query+' " WHERE ID=13885"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if((Get-Item "$repo/src/server/game/Maps/Map.cpp").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw 'Actual game library predates the Darkshore mount scope'}
 if((Get-Item "$repo/src/server/scripts/Kalimdor/zone_darkshore.cpp").LastWriteTimeUtc -gt (Get-Item $scriptsLibrary).LastWriteTimeUtc){throw 'Actual scripts library predates the Darkshore ride'}
}
if($OldFriends){
 $probeSource="$PSScriptRoot/OldFriendsNative.cpp"
 [IO.File]::WriteAllText("$out/NativeOldFriends.inc",$query+' " WHERE ID IN(24958,25023,25024,25184)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 foreach($file in @('Entities/Vehicle/OldFriendsFlight.h','Entities/Unit/Unit.cpp','Maps/Map.cpp')){
  if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
 }
 if((Get-Item "$repo/src/server/scripts/Maelstrom/LostIsles/lost_isles.cpp").LastWriteTimeUtc -gt (Get-Item $scriptsLibrary).LastWriteTimeUtc){throw 'Actual scripts library predates Lost Isles'}
}
if($Volcanoth -or $VolcanothBaseline){
 $probeSource="$PSScriptRoot/VolcanothNative.cpp"
 [IO.File]::WriteAllText("$out/NativeVolcanoth.inc",$query+' " WHERE ID IN(24929,24954,24958,25023,25184)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($Volcanoth){
  foreach($file in @('Entities/GameObject/QuestItemObjectUse.h','Spells/Spell.cpp','Spells/SpellEffects.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($FromTheShadows -or $FromTheShadowsBaseline){
 $probeSource="$PSScriptRoot/FromTheShadowsNative.cpp"
 [IO.File]::WriteAllText("$out/NativeFromTheShadows.inc",$query+' " WHERE ID IN(14159,14204,14214)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($FromTheShadows){
  foreach($file in @('Entities/GameObject/QuestItemObjectUse.h','Entities/Creature/TemporarySummon.cpp','Maps/Map.cpp','Spells/Spell.cpp','Spells/SpellEffects.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
  if((Get-Item "$repo/src/server/scripts/EasternKingdoms/Gilneas/gilneas.cpp").LastWriteTimeUtc -gt (Get-Item $scriptsLibrary).LastWriteTimeUtc){throw 'Actual scripts library predates Gilneas'}
 }
}
if($WoodsEndCoffer -or $WoodsEndCofferBaseline){
 $probeSource="$PSScriptRoot/WoodsEndCofferNative.cpp"
 [IO.File]::WriteAllText("$out/NativeWoodsEndCoffer.inc",$query+' " WHERE ID IN(14386,24593,24646)"')
 [IO.File]::WriteAllText("$out/NativeKorothBanner.inc",$query+' " WHERE ID IN(14386,24468,24472,24483)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($WoodsEndCoffer){
  foreach($file in @('Entities/GameObject/GameObject.cpp','Entities/GameObject/QuestItemObjectUse.h','Entities/Player/Player.cpp','Maps/Map.cpp','Spells/Spell.cpp','Spells/SpellEffects.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($KorothBanner -or $KorothBannerBaseline){
 $probeSource="$PSScriptRoot/KorothBannerNative.cpp"
 [IO.File]::WriteAllText("$out/NativeKorothBanner.inc",$query+' " WHERE ID IN(14386,24468,24472,24483)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($KorothBanner){
  foreach($file in @('Entities/GameObject/GameObject.cpp','Entities/GameObject/QuestItemObjectUse.h','Entities/Player/Player.cpp','Spells/SpellEffects.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($SwiftclawRide -or $SwiftclawRideBaseline){
 $probeSource="$PSScriptRoot/SwiftclawRideNative.cpp"
 [IO.File]::WriteAllText("$out/NativeSwiftclawRide.inc",$query+' " WHERE ID IN(24623,24626)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($SwiftclawRide){
  if((Get-Item "$repo/src/server/scripts/Kalimdor/zone_durotar.cpp").LastWriteTimeUtc -gt (Get-Item $scriptsLibrary).LastWriteTimeUtc){
   throw 'Actual scripts library predates zone_durotar.cpp'
  }
  if((Get-Item "$repo/src/server/game/Maps/Map.cpp").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw 'Actual game library predates Map.cpp'}
 }
}
if($PerfumedGuardAnalysis -or $PerfumedGuardAnalysisBaseline){
 $probeSource="$PSScriptRoot/PerfumedGuardAnalysisNative.cpp"
 [IO.File]::WriteAllText("$out/NativePerfumedGuardAnalysis.inc",$query+' " WHERE ID IN(24536,24655,24804,24805)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($PerfumedGuardAnalysis){
  foreach($file in @('Spells/Spell.cpp','Spells/SpellEffects.cpp','Spells/PerfumedGuardAnalysis.h')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
  if((Get-Item "$repo/src/server/scripts/World/guards.cpp").LastWriteTimeUtc -gt (Get-Item $scriptsLibrary).LastWriteTimeUtc){throw 'Actual scripts library predates guards.cpp'}
 }
}
if($KneeDeep -or $KneeDeepBaseline){
 $probeSource="$PSScriptRoot/KneeDeepNative.cpp"
 [IO.File]::WriteAllText("$out/NativeKneeDeep.inc",$query+' " WHERE ID IN(24678,24920,24903,24602)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($KneeDeep){
  foreach($file in @('Entities/GameObject/QuestItemObjectUse.h','Spells/Spell.cpp','Spells/SpellEffects.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($IncendiciteOre -or $IncendiciteOreBaseline){
 $probeSource="$PSScriptRoot/IncendiciteOreNative.cpp"
 [IO.File]::WriteAllText("$out/NativeIncendiciteOre.inc",$query+' " WHERE ID IN(25725,25735)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($IncendiciteOre){
  foreach($file in @('Entities/GameObject/GameObject.cpp','Entities/GameObject/QuestItemObjectUse.h','Entities/Player/Player.cpp','Spells/Spell.cpp','Spells/SpellEffects.cpp','Handlers/SpellHandler.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($LiberationDay -or $LiberationDayBaseline){
 $probeSource="$PSScriptRoot/LiberationDayNative.cpp"
 [IO.File]::WriteAllText("$out/NativeLiberationDay.inc",$query+' " WHERE ID IN(24575,24677)"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($LiberationDay){
  foreach($file in @('Entities/GameObject/GameObject.cpp','Entities/GameObject/QuestItemObjectUse.h','Spells/Spell.cpp','Spells/SpellEffects.cpp','Handlers/SpellHandler.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($IronforgeRations -or $IronforgeRationsBaseline){
 $probeSource="$PSScriptRoot/IronforgeRationsNative.cpp"
 [IO.File]::WriteAllText("$out/NativeIronforgeRations.inc",$query+' " WHERE ID=29351"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($IronforgeRations){
  foreach($file in @('Entities/GameObject/QuestItemObjectUse.h','Spells/Spell.cpp','Spells/SpellEffects.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($IronforgeBait -or $IronforgeBaitBaseline){
 $probeSource="$PSScriptRoot/IronforgeBaitNative.cpp"
 [IO.File]::WriteAllText("$out/NativeIronforgeBait.inc",$query+' " WHERE ID=29350"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($IronforgeBait){
  foreach($file in @('Entities/GameObject/GameObject.cpp','Entities/GameObject/QuestItemObjectUse.h','Entities/Player/Player.cpp','Spells/Spell.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($DarkmoonBanner -or $DarkmoonBannerBaseline){
 $probeSource="$PSScriptRoot/DarkmoonBannerNative.cpp"
 [IO.File]::WriteAllText("$out/NativeDarkmoonBanner.inc",$query+' " WHERE ID=29520"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($DarkmoonBanner){
  foreach($file in @('Entities/GameObject/GameObject.cpp','Entities/GameObject/QuestItemObjectUse.h','Entities/Player/Player.cpp','Spells/Spell.cpp','Spells/Spell.h','Spells/SpellEffects.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($DarkmoonFairTargets -or $DarkmoonFairTargetsBaseline){
 $probeSource="$PSScriptRoot/DarkmoonFairTargetsNative.cpp"
 [IO.File]::WriteAllText("$out/NativeDarkmoonFairTargets.inc",$query+' " WHERE ID IN (29509,29511) ORDER BY ID"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($DarkmoonFairTargets){
  foreach($file in @('Entities/Player/Player.cpp','Spells/Spell.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($DarkmoonCrafting -or $DarkmoonCraftingBaseline){
 $probeSource="$PSScriptRoot/DarkmoonCraftingNative.cpp"
 [IO.File]::WriteAllText("$out/NativeDarkmoonCrafting.inc",$query+' " WHERE ID IN (29506,29510,29515,29516,29517) ORDER BY ID"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($DarkmoonCrafting){
  foreach($file in @('Entities/GameObject/GameObject.cpp','Entities/GameObject/QuestItemObjectUse.h','Entities/Player/Player.cpp','Spells/Spell.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
}
if($LovelyCharmGifts -or $LovelyCharmGiftsBaseline){
 $probeSource = "$PSScriptRoot/LovelyCharmGiftsNative.cpp"
 [IO.File]::WriteAllText("$out/NativeLovelyCharmGifts.inc",$query+' " WHERE ID IN (24597,24609,24610,24611,24612,24613,24614,24615) ORDER BY ID"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($LovelyCharmGifts -and (Get-Item "$repo/src/server/game/Entities/Player/Player.cpp").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw 'Actual game library predates the gift cleanup and season checks'}
}
if($ThelgenSeismograph -or $ThelgenSeismographBaseline){
 $probeSource = "$PSScriptRoot/ThelgenSeismographNative.cpp"
 [IO.File]::WriteAllText("$out/NativeThelgenSeismograph.inc",$query+' " WHERE ID IN (25726,25734) ORDER BY ID"')
 [IO.File]::WriteAllText("$out/NativeQuestSupplyChests.inc",$query+' " WHERE ID IN (13909,28325,29360,29418)"')
 if($ThelgenSeismograph){
  foreach($file in @('Entities/GameObject/QuestItemObjectUse.h','Entities/GameObject/GameObject.cpp','Entities/Player/Player.cpp','Globals/QuestData.cpp','Spells/Spell.cpp')){
   if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
  }
 }
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
if($IncendiciteOreBaseline){$compilerArgs+='/DINCENDICITE_ORE_BASELINE'}
if($KneeDeepBaseline){$compilerArgs+='/DKNEE_DEEP_BASELINE'}
if($PerfumedGuardAnalysisBaseline){$compilerArgs+='/DPERFUMED_GUARD_ANALYSIS_BASELINE'}
if($SwiftclawRideBaseline){$compilerArgs+='/DSWIFTCLAW_RIDE_BASELINE'}
if($KezanHotRodBaseline){$compilerArgs+='/DKEZAN_HOT_ROD_BASELINE'}
if($ForestEttinBaseline){$compilerArgs+='/DFOREST_ETTIN_BASELINE'}
if($HuoOfferingsBaseline){$compilerArgs+='/DHUO_OFFERINGS_BASELINE'}
if($SingingPoolsBaseline){$compilerArgs+='/DSINGING_POOLS_BASELINE'}
if($SingingPoolsWaterBaseline){$compilerArgs+='/DSINGING_POOLS_WATER_BASELINE'}
if($SingingPoolsReedsBaseline){$compilerArgs+='/DSINGING_POOLS_REEDS_BASELINE'}
if($SingingPoolsRockBaseline){$compilerArgs+='/DSINGING_POOLS_ROCK_BASELINE'}
if($SingingPoolsPearlBaseline){$compilerArgs+='/DSINGING_POOLS_PEARL_BASELINE'}
if($SingingPoolsShuAdmissionBaseline){$compilerArgs+='/DSINGING_POOLS_SHU_ADMISSION_BASELINE'}
if($SingingPoolsReflectionBaseline){$compilerArgs+='/DSINGING_POOLS_REFLECTION_BASELINE'}
if($SingingPoolsCoaxBaseline){$compilerArgs+='/DSINGING_POOLS_COAX_BASELINE'}
if($SingingPoolsSpoutBaseline){$compilerArgs+='/DSINGING_POOLS_SPOUT_BASELINE'}
if($SingingPoolsCartBaseline){$compilerArgs+='/DSINGING_POOLS_CART_BASELINE'}
if($SingingPoolsFarmBaseline){$compilerArgs+='/DSINGING_POOLS_FARM_BASELINE'}
if($SingingPoolsMalletBaseline){$compilerArgs+='/DSINGING_POOLS_MALLET_BASELINE'}
if($SingingPoolsGongBaseline){$compilerArgs+='/DSINGING_POOLS_GONG_BASELINE'}
if($SingingPoolsWugouBaseline){$compilerArgs+='/DSINGING_POOLS_WUGOU_BASELINE'}
if($SingingPoolsScrollBaseline){$compilerArgs+='/DSINGING_POOLS_SCROLL_BASELINE'}
if($SingingPoolsBreezeSuppliesBaseline){$compilerArgs+='/DSINGING_POOLS_BREEZE_SUPPLIES_BASELINE'}
if($SingingPoolsFeFengBaseline){$compilerArgs+='/DSINGING_POOLS_FE_FENG_BASELINE'}
if($SingingPoolsRuffianBaseline){$compilerArgs+='/DSINGING_POOLS_RUFFIAN_BASELINE'}
if($SingingPoolsWindEscortBaseline){$compilerArgs+='/DSINGING_POOLS_WIND_ESCORT_BASELINE'}
if($SingingPoolsZhaoBaseline){$compilerArgs+='/DSINGING_POOLS_ZHAO_BASELINE'}
if($PassingWisdomBaseline){$compilerArgs+='/DPASSING_WISDOM_BASELINE'}
if($PassingWisdomOrphanBaseline){$compilerArgs+='/DPASSING_WISDOM_ORPHAN_BASELINE'}
if($WanderingIsleBalloon){$compilerArgs+='/DWANDERING_ISLE_BALLOON'}
if($WanderingIsleBalloonBaseline){$compilerArgs+='/DWANDERING_ISLE_BALLOON_BASELINE'}
if($WanderingIsleMandori){$compilerArgs+='/DWANDERING_ISLE_MANDORI'}
if($WanderingIsleMandoriBaseline){$compilerArgs+='/DWANDERING_ISLE_MANDORI_BASELINE'}
if($WanderingIsleSouthBaseline){$compilerArgs+='/DWANDERING_ISLE_SOUTH_BASELINE'}
if($WanderingIsleMedicalBaseline){$compilerArgs+='/DWANDERING_ISLE_MEDICAL_BASELINE'}
if($WanderingIsleRescue -or $WanderingIsleRescueBaseline){$compilerArgs+='/DWANDERING_ISLE_RESCUE'}
if($WanderingIsleRescueBaseline){$compilerArgs+='/DWANDERING_ISLE_RESCUE_BASELINE'}
if($WanderingIsleThorn -or $WanderingIsleThornBaseline){$compilerArgs+='/DWANDERING_ISLE_THORN'}
if($WanderingIsleThornBaseline){$compilerArgs+='/DWANDERING_ISLE_THORN_BASELINE'}
if($WanderingIsleHealing -or $WanderingIsleHealingBaseline){$compilerArgs+='/DWANDERING_ISLE_HEALING'}
if($WanderingIsleHealingBaseline){$compilerArgs+='/DWANDERING_ISLE_HEALING_BASELINE'}
if($WanderingIsleDeparture){$compilerArgs+='/DWANDERING_ISLE_DEPARTURE'}
if($WanderingIsleDepartureBaseline){$compilerArgs+='/DWANDERING_ISLE_DEPARTURE_BASELINE'}
if($ChallengerFiresBaseline){$compilerArgs+='/DCHALLENGER_FIRES_BASELINE'}
if($IlgalarWardBaseline){$compilerArgs+='/DILGALAR_WARD_BASELINE'}
if($BlackrockPrisonersBaseline){$compilerArgs+='/DBLACKROCK_PRISONERS_BASELINE'}
if($RedridgeSeaforiumBaseline){$compilerArgs+='/DREDRIDGE_SEAFORIUM_BASELINE'}
if($StaggeringEffortBaseline){$compilerArgs+='/DSTAGGERING_EFFORT_BASELINE'}
if($DownWithCrushcogBaseline){$compilerArgs+='/DDOWN_WITH_CRUSHCOG_BASELINE'}
if($PaintItBlackBaseline){$compilerArgs+='/DPAINT_IT_BLACK_BASELINE'}
if($NoTanksBaseline){$compilerArgs+='/DNO_TANKS_BASELINE'}
if($WestfallVisionObserve){$compilerArgs+='/DWESTFALL_VISION_OBSERVE'}
if($WestfallVisionScene){$compilerArgs+='/DWESTFALL_VISION_SCENE'}
if($DeeprunRatObserve){$compilerArgs+='/DDEEPRUN_RAT_OBSERVE'}
if($HighestQuestPOI){$compilerArgs+='/DCHECK_HIGHEST_QUEST_POI'}
if($OslowQuestPOI){$compilerArgs+='/DCHECK_OSLOW_QUEST_POI'}
if($MoonkinQuestPOI){$compilerArgs+='/DCHECK_MOONKIN_QUEST_POI'}
if($CameraQuestPOI){$compilerArgs+='/DCHECK_CAMERA_QUEST_POI'}
if($VisionQuestPOI){$compilerArgs+='/DCHECK_VISION_QUEST_POI'}
if($LegacyQuestPOI){$compilerArgs+='/DCHECK_LEGACY_QUEST_POI'}
if($WestfallStewBaseline){$compilerArgs+='/DWESTFALL_STEW_BASELINE'}
if($PricelessTreasuresBaseline){$compilerArgs+='/DPRICELESS_TREASURES_BASELINE'}
if($ScorpidVenomBaseline){$compilerArgs+='/DSCORPID_VENOM_BASELINE'}
if($VolcanothBaseline){$compilerArgs+='/DVOLCANOTH_BASELINE'}
if($FromTheShadowsBaseline){$compilerArgs+='/DFROM_THE_SHADOWS_BASELINE'}
if($WoodsEndCofferBaseline){$compilerArgs+='/DWOODS_END_COFFER_BASELINE'}
if($KorothBannerBaseline){$compilerArgs+='/DKOROTH_BANNER_BASELINE'}
if($LiberationDayBaseline){$compilerArgs+='/DLIBERATION_DAY_BASELINE'}
if($IronforgeRationsBaseline){$compilerArgs+='/DIRONFORGE_RATIONS_BASELINE'}
if($IronforgeBaitBaseline){$compilerArgs+='/DIRONFORGE_BAIT_BASELINE'}
if($DarkmoonBannerBaseline){$compilerArgs+='/DDARKMOON_BANNER_BASELINE'}
if($DarkmoonFairTargetsBaseline){$compilerArgs+='/DDARKMOON_FAIR_TARGETS_BASELINE'}
if($DarkmoonCraftingBaseline){$compilerArgs+='/DDARKMOON_CRAFTING_BASELINE'}
if($LovelyCharmGiftsBaseline){$compilerArgs+='/DLOVELY_CHARM_GIFTS_BASELINE'}
if($ThelgenSeismographBaseline){$compilerArgs+='/DTHELGEN_SEISMOGRAPH_BASELINE'}
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
if($DreadlordsPrizePortalBaseline){$config+="`r`nNativeDreadlordsPrize.ExpectOldAction = 1"}
if($SunGateBaseline){$config+="`r`nNativeSunGate.ExpectBroken = 1"}
if($AzsharaDetonatorBaseline){$config+="`r`nNativeAzsharaDetonator.ExpectBroken = 1"}
[IO.File]::WriteAllText("$run/worldserver.conf",$config)
$prior=$env:PATH
try{
 $env:PATH="$runtime;$prior";Push-Location $runtime
 try{& "$out/probe.exe" "$run/worldserver.conf" "$runtime/" > "$run/stdout.log" 2> "$run/stderr.log";$exit=$LASTEXITCODE}finally{Pop-Location}
}finally{$env:PATH=$prior}
Get-Content "$run/stdout.log"|Where-Object {$_ -match '^(PASS|EXPECTED_BROKEN|CASE)'}
$expectedAmaniDiagnostic = $AmaniCatacombsBaseline -and
    [IO.File]::ReadAllText("$run/stderr.log").Trim() -ceq 'ERROR [entities.player] Deprecated function AreaExploredOrEventHappens called for quest 9193'
$dbErrorFile = Get-Item -LiteralPath "$run/DBErrors.log" -ErrorAction SilentlyContinue
$dbErrors = if($dbErrorFile){[IO.File]::ReadAllText($dbErrorFile.FullName).Trim()}else{''}
$portalDiagnostic = 'ERROR [sql.sql] SmartScript: Action target Creature(entry: 0) is not using SmartAI, action skipped to prevent crash.'
$expectedPortalDiagnostic = $DreadlordsPrizePortalBaseline -and
    ($dbErrors -creplace '^\d{4}-\d\d-\d\d_\d\d:\d\d:\d\d ', '') -ceq $portalDiagnostic
if($exit -or ((Get-Item "$run/stderr.log").Length -and !$expectedAmaniDiagnostic -and
    !($expectedPortalDiagnostic -and [IO.File]::ReadAllText("$run/stderr.log").Trim() -ceq $portalDiagnostic))){
 Get-Content "$run/stderr.log" -Tail 12;throw "Native quest-object loot exit=$exit or stderr"
}
if($expectedAmaniDiagnostic){Get-Content "$run/stderr.log"}
if($DreadlordsPrizePortalBaseline -and !$expectedPortalDiagnostic){throw 'The pre-migration portal did not reproduce exactly one expected DB error'}
if($expectedPortalDiagnostic){Write-Output "EXPECTED_BROKEN $dbErrors"}
elseif($dbErrorFile -and $dbErrorFile.Length){throw 'Native loot emitted DBErrors; inspect preserved evidence'}
@{baseline=[bool]($Baseline -or $DreadlordsPrizePortalBaseline -or $SunGateBaseline -or $AzsharaDetonatorBaseline -or $WanderingIsleDepartureBaseline -or $WanderingIsleHealingBaseline -or $WanderingIsleThornBaseline -or $WanderingIsleRescueBaseline -or $WanderingIsleMedicalBaseline -or $WanderingIsleSouthBaseline -or $WanderingIsleMandoriBaseline -or $WanderingIsleBalloonBaseline -or $PassingWisdomBaseline -or $PassingWisdomOrphanBaseline -or $SingingPoolsBaseline -or $SingingPoolsWaterBaseline -or $SingingPoolsReedsBaseline -or $SingingPoolsRockBaseline -or $SingingPoolsPearlBaseline -or $SingingPoolsShuAdmissionBaseline -or $SingingPoolsReflectionBaseline -or $SingingPoolsCoaxBaseline -or $SingingPoolsSpoutBaseline -or $SingingPoolsCartBaseline -or $SingingPoolsFarmBaseline -or $SingingPoolsMalletBaseline -or $SingingPoolsGongBaseline -or $SingingPoolsWugouBaseline -or $SingingPoolsScrollBaseline -or $SingingPoolsBreezeSuppliesBaseline -or $SingingPoolsFeFengBaseline -or $SingingPoolsRuffianBaseline -or $SingingPoolsWindEscortBaseline -or $SingingPoolsZhaoBaseline -or $SeasoningBaseline -or $AquantionBaseline -or $AnowynBaseline -or $AlliedCityBaseline -or $DuskwitherBaseline -or $RedSnapperBaseline -or $SwiftDisciplineBaseline -or $ExtinguishingHopeBaseline -or $MulgoreCagesBaseline -or $AshenvaleFelWoodBaseline -or $ElwynnQuestsBaseline -or $SupplyChestsBaseline -or $ForestHeartBaseline -or $StillpinePrimerBaseline -or $MonkeyBusinessBaseline -or $MonkeyBusinessFlightBaseline -or $HogtiedRazormaneBaseline -or $TongaGuideBaseline -or $AirfieldFirefightingBaseline -or $FelfireSeedlingsBaseline -or $BattleboarTroughsBaseline -or $BalboaBasaltBaseline -or $VileFinLeashBaseline -or $AbsorbentBaseline -or $SfgMinefieldBaseline -or $DozercismBaseline -or $StagnantOasisBaseline -or $AmaniCatacombsBaseline -or $RavagerCageBaseline -or $AkidaCagesBaseline -or $PrincessStillpineBaseline -or $BloodcupBraidBaseline -or $SamophlangeManualBaseline -or $GutsAndGoreBaseline -or $ThelgenSeismographBaseline -or $LovelyCharmGiftsBaseline -or $DarkmoonCraftingBaseline -or $DarkmoonFairTargetsBaseline -or $DarkmoonBannerBaseline -or $IronforgeBaitBaseline -or $IronforgeRationsBaseline -or $LiberationDayBaseline -or $IncendiciteOreBaseline -or $KneeDeepBaseline -or $PerfumedGuardAnalysisBaseline -or $SwiftclawRideBaseline -or $KorothBannerBaseline -or $WoodsEndCofferBaseline -or $FromTheShadowsBaseline -or $VolcanothBaseline -or $ScorpidVenomBaseline -or $PricelessTreasuresBaseline -or $WestfallStewBaseline -or $NoTanksBaseline -or $PaintItBlackBaseline -or $DownWithCrushcogBaseline -or $StaggeringEffortBaseline -or $RedridgeSeaforiumBaseline -or $BlackrockPrisonersBaseline -or $IlgalarWardBaseline -or $ChallengerFiresBaseline -or $HuoOfferingsBaseline -or $ForestEttinBaseline -or $KezanHotRodBaseline);game=(Get-FileHash $game).Hash;scripts=(Get-FileHash $scriptsLibrary).Hash;executable=(Get-FileHash "$out/probe.exe").Hash;source=(Get-FileHash $probeSource).Hash;run=$run;exit=$exit}|ConvertTo-Json|Set-Content "$out/native-inputs.json"
'PASS native execution/exit/stdout/stderr/DBErrors; wrapper must also inspect crash and appended server diagnostics'
