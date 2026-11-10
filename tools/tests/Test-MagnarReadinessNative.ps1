# Real native Protection wave readiness and Kill/owner progression; no persistent character writes.
param([ValidateSet("HealthDeltaNative","FreyaElderHealthNative","TsulongTransitionNative","KarazhanRookNative","VehicleDeferredAdmissionNative","SevenSpellLifecycleNative","WarswordsStageFourNative","TenSpellLifecycleNative","NextTenSpellNative","ThirdTenSpellNative","FourthTenSpellNative","FifthTenNative","SixthTenNative","SeventhTenNative","EighthTenNative","NinthTenNative","TenthTenNative","EleventhTenNative","TwelfthTenNative","ThirteenthTenNative","FourteenthTenNative","FifteenthTenNative","SixteenthTenNative","SeventeenthTenNative","EighteenthTenNative","NineteenthTenNative","TwentiethTenNative","TwentyFirstTenNative","TwentySecondTenNative","TwentyThirdTenNative","TwentyFourthTenNative","TwentyFifthTenNative","TwentySixthTenNative","TwentySeventhTenNative","TwentyEighthTenNative","TwentyNinthTenNative","ThirtiethTenNative","ThirtyFirstTenNative","ThirtySecondTenNative","ThirtyThirdTenNative","ThirtyFourthTenNative","ThirtyFifthTenNative","ThirtySixthTenNative")][string]$AdditionalProbe,[switch]$AdditionalBaseline,[switch]$SharasdalLadyCarryLive,[switch]$VehicleAuraCapacityBaseline,[switch]$VehicleAuraCapacity,[switch]$SharasdalLadyCarry,[switch]$SharasdalWaveMotion,[switch]$SharasdalWaveContact,[switch]$SharasdalWaveBaseline,[switch]$SharasdalWave,[switch]$DevourerOutroBaseline,[switch]$DevourerOutro,[switch]$DevourerGraveyardBaseline,[switch]$DevourerGraveyard,[switch]$KingslayersGraveyardBaseline,[switch]$KingslayersGraveyard,[switch]$DreadbladesGraveyardBaseline,[switch]$DreadbladesGraveyard,[switch]$SharasdalEntranceBaseline,[switch]$SharasdalEntranceLive,[switch]$SharasdalEntrance,[switch]$SharasdalGraveyardBaseline,[switch]$SharasdalGraveyard,[switch]$HolyGraveyardBaseline,[switch]$HolyGraveyard,[switch]$ShadowGraveyardBaseline,[switch]$ShadowGraveyard,[switch]$ArmsGraveyardBaseline,[switch]$ArmsGraveyard,[switch]$HodirGraveyardBaseline,[switch]$HodirGraveyard,[switch]$HodirLandingBaseline,[switch]$HodirLandingLive,[switch]$HodirLanding,[switch]$BloodGraveyardBaseline,[switch]$BloodGraveyard,[switch]$MarksmanGraveyardBaseline,[switch]$MarksmanGraveyard,[switch]$TuureGraveyardBaseline,[switch]$TuureGraveyard,[switch]$TuureLegacyBaseline,[switch]$TuureLegacy,[switch]$TuureEntranceBaseline,[switch]$TuureEntranceLive,[switch]$TuureEntrance,[switch]$BloodEntranceBaseline,[switch]$BloodEntranceLive,[switch]$BloodEntrance,[switch]$MarksmanEarned,[switch]$MarksmanRecoveryBaseline,[switch]$MarksmanRecovery,[switch]$MarksmanshipEntranceBaseline,[switch]$MarksmanshipEntranceLive,[switch]$MarksmanshipEntrance,[switch]$HunterRecoveryBaseline,[switch]$HunterRecovery,[switch]$SurvivalEntranceBaseline,[switch]$SurvivalEntranceLive,[switch]$SurvivalEntrance,[switch]$SurvivalGraveyardBaseline,[switch]$SurvivalGraveyard,[switch]$FuryGraveyardBaseline,[switch]$FuryGraveyard,[switch]$WarriorRecoveryBaseline,[switch]$WarriorRecovery,[switch]$WarriorLoginBaseline,[switch]$WarriorLogin,[switch]$LoginBaseline,[switch]$LoginLiveData,[switch]$LoginRecovery,[switch]$DuelDeferredBaseline,[switch]$DuelRecreation,[switch]$DuelRecreationBaseline,[switch]$Recreation,[switch]$RecreationBaseline,[switch]$CombatReplacement,[switch]$CombatBaseline,[switch]$PrimaryApproach,[switch]$PrimaryBaseline,[switch]$Cursor,[switch]$CursorBaseline,[switch]$Replacement,[switch]$ReplacementBaseline,[switch]$ColdRoute,[switch]$ColdRouteBaseline,[switch]$Baseline,[switch]$StartRecovery,[switch]$Pillik,[switch]$PillikBaseline,[switch]$EntryBaseline,[switch]$Approach,[switch]$ApproachBaseline,[switch]$Reoffer,[switch]$ReofferBaseline,[switch]$ResetRecovery,[switch]$ResetBaseline,[switch]$ChamberRecovery,[switch]$ChamberBaseline,[switch]$Barrier,[switch]$BarrierBaseline)
if($VehicleAuraCapacityBaseline){$VehicleAuraCapacity=$true}
if($VehicleAuraCapacity){if(@($PSBoundParameters.Keys|Where-Object {$_ -notin @('VehicleAuraCapacity','VehicleAuraCapacityBaseline')}).Count){throw 'Choose only native vehicle capacity scope'}}
if($SharasdalLadyCarryLive){$SharasdalLadyCarry=$true}
if($SharasdalLadyCarry){if(@($PSBoundParameters.Keys|Where-Object {$_ -notin @('SharasdalLadyCarry','SharasdalLadyCarryLive')}).Count){throw 'Choose only Sharasdal carry scope'}}
if($SharasdalWaveMotion){if(@($PSBoundParameters.Keys|Where-Object {$_ -ne 'SharasdalWaveMotion'}).Count){throw 'Choose only Sharasdal motion scope'}}
if($SharasdalWaveContact){if(@($PSBoundParameters.Keys|Where-Object {$_ -ne 'SharasdalWaveContact'}).Count){throw 'Choose only Sharasdal contact scope'}}
if($SharasdalWaveBaseline){$SharasdalWave=$true}
if($SharasdalWave){if(@($PSBoundParameters.Keys|Where-Object {$_ -notin @('SharasdalWave','SharasdalWaveBaseline')}).Count){throw 'Choose only Sharasdal wave scope'}}
if($DevourerOutroBaseline){$DevourerOutro=$true}
if($DevourerOutro){if(@($PSBoundParameters.Keys|Where-Object {$_ -notin @('DevourerOutro','DevourerOutroBaseline')}).Count){throw 'Choose only Devourer outro scope'}}
if($DevourerGraveyardBaseline){$DevourerGraveyard=$true}
if($DevourerGraveyard){if(@($PSBoundParameters.Keys|Where-Object {$_ -notin @('DevourerGraveyard','DevourerGraveyardBaseline')}).Count){throw 'Choose only Devourer cemetery scope'}}
if($KingslayersGraveyardBaseline){$KingslayersGraveyard=$true}
if($KingslayersGraveyard){if(@($PSBoundParameters.Keys|Where-Object {$_ -notin @('KingslayersGraveyard','KingslayersGraveyardBaseline')}).Count){throw 'Choose only Kingslayers cemetery scope'}}
if($DreadbladesGraveyardBaseline){$DreadbladesGraveyard=$true}
if($DreadbladesGraveyard){if(@($PSBoundParameters.Keys|Where-Object {$_ -notin @('DreadbladesGraveyard','DreadbladesGraveyardBaseline')}).Count){throw 'Choose only Dreadblades cemetery scope'};$SurvivalGraveyard=$true}
if($SharasdalEntranceBaseline -and $SharasdalEntranceLive){throw 'Choose pinned legacy or live Sharasdal data'}
if($SharasdalEntranceBaseline -or $SharasdalEntranceLive){$SharasdalEntrance=$true}
if($SharasdalEntrance){if(@($PSBoundParameters.Keys|Where-Object {$_ -notin @('SharasdalEntrance','SharasdalEntranceBaseline','SharasdalEntranceLive')}).Count){throw 'Choose only Sharasdal entrance scope'};$SurvivalGraveyard=$true}
if($SharasdalGraveyardBaseline){$SharasdalGraveyard=$true}
if($SharasdalGraveyard){if(@($PSBoundParameters.Keys|Where-Object {$_ -notin @('SharasdalGraveyard','SharasdalGraveyardBaseline')}).Count){throw 'Choose only Sharasdal cemetery scope'};$SurvivalGraveyard=$true}
if($HolyGraveyardBaseline){$HolyGraveyard=$true}
if($HolyGraveyard){if(@($PSBoundParameters.Keys|Where-Object {$_ -notin @('HolyGraveyard','HolyGraveyardBaseline')}).Count){throw 'Choose only Holy cemetery scope'};$SurvivalGraveyard=$true}
if($ShadowGraveyardBaseline){$ShadowGraveyard=$true}
if($ShadowGraveyard){if(@($PSBoundParameters.Keys|Where-Object {$_ -notin @('ShadowGraveyard','ShadowGraveyardBaseline')}).Count){throw 'Choose only Shadow cemetery scope'};$SurvivalGraveyard=$true}
if($ArmsGraveyardBaseline){$ArmsGraveyard=$true}
if($ArmsGraveyard){if(@($PSBoundParameters.Keys|Where-Object {$_ -notin @('ArmsGraveyard','ArmsGraveyardBaseline')}).Count){throw 'Choose only Arms cemetery scope'};$SurvivalGraveyard=$true}
if($HodirGraveyardBaseline){$HodirGraveyard=$true}
if($HodirGraveyard){if(@($PSBoundParameters.Keys|Where-Object {$_ -notin @('HodirGraveyard','HodirGraveyardBaseline')}).Count){throw 'Choose only Hodir cemetery scope'};$TuureGraveyard=$true}
if($HodirLandingBaseline -and $HodirLandingLive){throw 'Choose Hodir component counterfactual or live graph'}
if($HodirLandingBaseline -or $HodirLandingLive){$HodirLanding=$true}
if($HodirLanding){if(@($PSBoundParameters.Keys|Where-Object {$_ -notin @('HodirLanding','HodirLandingBaseline','HodirLandingLive')}).Count){throw 'Choose only Hodir landing scope'};$SurvivalGraveyard=$true}
if($BloodGraveyardBaseline){$BloodGraveyard=$true}
if($BloodGraveyard){$TuureGraveyard=$true}
if($MarksmanGraveyardBaseline){$MarksmanGraveyard=$true}
if($MarksmanGraveyard){$TuureGraveyard=$true}
if($TuureGraveyardBaseline){$TuureGraveyard=$true}
if($TuureGraveyard){$SurvivalGraveyard=$true}
if($TuureLegacyBaseline){$TuureLegacy=$true}
if($TuureLegacy){$TuureEntrance=$true}
if($TuureEntranceBaseline -and $TuureEntranceLive){throw 'Choose Tuure legacy baseline or live data'}
if($TuureEntranceBaseline -or $TuureEntranceLive){$TuureEntrance=$true}
if($TuureEntrance){$SurvivalGraveyard=$true}
if($BloodEntranceBaseline -and $BloodEntranceLive){throw 'Choose missing baseline or live Blood data'}
if($BloodEntranceBaseline -or $BloodEntranceLive){$BloodEntrance=$true}
if($BloodEntrance){$SurvivalGraveyard=$true}
if($MarksmanEarned){$MarksmanRecovery=$true}

if($MarksmanRecoveryBaseline){$MarksmanRecovery=$true}
if($MarksmanRecovery){if($HunterRecovery -or $HunterRecoveryBaseline -or $MarksmanshipEntrance -or $MarksmanshipEntranceBaseline -or $MarksmanshipEntranceLive -or $WarriorRecoveryBaseline){throw 'Choose only Marksmanship login recovery'};$SurvivalGraveyard=$true}

if($MarksmanshipEntranceBaseline -and $MarksmanshipEntranceLive){throw 'Choose TEMP missing baseline or live data'}
if($MarksmanshipEntranceBaseline -or $MarksmanshipEntranceLive){$MarksmanshipEntrance=$true}
if($MarksmanshipEntrance){if($HunterRecovery -or $HunterRecoveryBaseline -or $SurvivalEntrance -or $SurvivalEntranceBaseline -or $SurvivalEntranceLive){throw 'Choose only Marksmanship entrance'};$SurvivalGraveyard=$true}

if($HunterRecoveryBaseline){$HunterRecovery=$true}
if($HunterRecovery){if($WarriorRecoveryBaseline -or $SurvivalGraveyardBaseline -or $FuryGraveyardBaseline){throw 'Choose only Hunter login recovery'};$SurvivalGraveyard=$true}
if($SurvivalEntranceBaseline -and $SurvivalEntranceLive){throw 'Choose TEMP missing baseline or live data'}
if($SurvivalEntranceBaseline -or $SurvivalEntranceLive){$SurvivalEntrance=$true}
if($SurvivalEntrance){$SurvivalGraveyard=$true}
if($SurvivalGraveyardBaseline){$SurvivalGraveyard=$true}
if($SurvivalGraveyard){if($FuryGraveyardBaseline){throw 'Choose distinct Hunter/Fury counterfactual'};$FuryGraveyard=$true}
if($FuryGraveyardBaseline){$FuryGraveyard=$true}
if($FuryGraveyard){$WarriorRecovery=$true}
if($WarriorRecoveryBaseline){$WarriorRecovery=$true}
if($WarriorRecovery){if($WarriorLoginBaseline -or $LoginBaseline -or $LoginLiveData){throw 'Choose only early Warrior recovery'};$WarriorLogin=$true}
if($WarriorLoginBaseline){$WarriorLogin=$true}
if($WarriorLogin){if($LoginBaseline -or $LoginLiveData){throw 'Choose only shared Warrior login'};$LoginRecovery=$true}
if($LoginBaseline -and $LoginLiveData){throw 'Choose one login mode'}
if($LoginBaseline -or $LoginLiveData){$LoginRecovery=$true}
if($LoginRecovery){if($DuelDeferredBaseline -or $DuelRecreationBaseline){throw 'Login recovery has no duel counterfactual'};$DuelRecreation=$true}
if($DuelDeferredBaseline -and $DuelRecreationBaseline){throw "Choose one duel counterfactual"}
if($DuelDeferredBaseline){$DuelRecreation=$true}
if($DuelRecreationBaseline){$DuelRecreation=$true}
if($DuelRecreation -and ($Recreation -or $RecreationBaseline -or $CombatReplacement -or $CombatBaseline -or $PrimaryApproach -or $PrimaryBaseline -or $Cursor -or $CursorBaseline -or $Replacement -or $ReplacementBaseline -or $ColdRoute -or $ColdRouteBaseline -or $StartRecovery -or $Baseline -or $Pillik -or $PillikBaseline -or $EntryBaseline -or $Approach -or $ApproachBaseline -or $Reoffer -or $ReofferBaseline -or $ResetRecovery -or $ResetBaseline -or $ChamberRecovery -or $ChamberBaseline -or $Barrier -or $BarrierBaseline)){throw "Choose only earned duel Creature recreation"}
if($RecreationBaseline){$Recreation=$true}
if($Recreation -and ($CombatReplacement -or $CombatBaseline -or $PrimaryApproach -or $PrimaryBaseline -or $Cursor -or $CursorBaseline -or $Replacement -or $ReplacementBaseline -or $ColdRoute -or $ColdRouteBaseline -or $StartRecovery -or $Baseline -or $Pillik -or $PillikBaseline -or $EntryBaseline -or $Approach -or $ApproachBaseline -or $Reoffer -or $ReofferBaseline -or $ResetRecovery -or $ResetBaseline -or $ChamberRecovery -or $ChamberBaseline -or $Barrier -or $BarrierBaseline)){throw "Choose only actual Magnar Creature recreation"}
if($CombatBaseline){$CombatReplacement=$true}
if($CombatReplacement -and ($PrimaryApproach -or $PrimaryBaseline -or $Cursor -or $CursorBaseline -or $Replacement -or $ReplacementBaseline -or $ColdRoute -or $ColdRouteBaseline -or $StartRecovery -or $Baseline -or $Pillik -or $PillikBaseline -or $EntryBaseline -or $Approach -or $ApproachBaseline -or $Reoffer -or $ReofferBaseline -or $ResetRecovery -or $ResetBaseline -or $ChamberRecovery -or $ChamberBaseline -or $Barrier -or $BarrierBaseline)){throw "Choose only earned Magnar combat replacement scope"}
if($PrimaryBaseline){$PrimaryApproach=$true}
if($PrimaryApproach -and ($Cursor -or $CursorBaseline -or $Replacement -or $ReplacementBaseline -or $ColdRoute -or $ColdRouteBaseline -or $StartRecovery -or $Baseline -or $Pillik -or $PillikBaseline -or $EntryBaseline -or $Approach -or $ApproachBaseline -or $Reoffer -or $ReofferBaseline -or $ResetRecovery -or $ResetBaseline -or $ChamberRecovery -or $ChamberBaseline -or $Barrier -or $BarrierBaseline)){throw 'Choose only original point6 approach scope'}
if($CursorBaseline){$Cursor=$true}
if($Cursor -and ($Replacement -or $ReplacementBaseline -or $ColdRoute -or $ColdRouteBaseline -or $StartRecovery -or $Baseline -or $Pillik -or $PillikBaseline -or $EntryBaseline -or $Approach -or $ApproachBaseline -or $Reoffer -or $ReofferBaseline -or $ResetRecovery -or $ResetBaseline -or $ChamberRecovery -or $ChamberBaseline -or $Barrier -or $BarrierBaseline)){throw 'Choose only cursor scope'}
if($ReplacementBaseline){$Replacement=$true}
if($Replacement -and ($ColdRoute -or $ColdRouteBaseline -or $Barrier -or $BarrierBaseline -or $ChamberRecovery -or $ChamberBaseline -or $ResetRecovery -or $ResetBaseline -or $Reoffer -or $ReofferBaseline -or $Approach -or $ApproachBaseline -or $StartRecovery -or $Pillik -or $PillikBaseline -or $EntryBaseline -or $Baseline)){throw 'Choose only the Magnar AI replacement native scope'}
if($ColdRouteBaseline){$ColdRoute=$true}
if($ColdRoute -and ($Barrier -or $BarrierBaseline -or $ChamberRecovery -or $ChamberBaseline -or $ResetRecovery -or $ResetBaseline -or $Reoffer -or $ReofferBaseline -or $Approach -or $ApproachBaseline -or $StartRecovery -or $Pillik -or $PillikBaseline -or $EntryBaseline -or $Baseline)){throw 'Choose only the cold chamber route native scope'}
if($BarrierBaseline){$Barrier=$true}
if($Barrier -and ($ChamberRecovery -or $ChamberBaseline -or $ResetRecovery -or $ResetBaseline -or $Reoffer -or $ReofferBaseline -or $Approach -or $ApproachBaseline -or $StartRecovery -or $Pillik -or $PillikBaseline -or $EntryBaseline -or $Baseline)){throw 'Choose only the Protection barrier native scope'}
if($ChamberBaseline){$ChamberRecovery=$true}
if($ChamberRecovery -and ($ResetRecovery -or $ResetBaseline -or $Reoffer -or $ReofferBaseline -or $Approach -or $ApproachBaseline -or $StartRecovery -or $Pillik -or $PillikBaseline -or $EntryBaseline -or $Baseline)){throw 'Choose only the fresh chamber native scope'}
if($ResetBaseline){$ResetRecovery=$true}
if($ResetRecovery -and ($Reoffer -or $ReofferBaseline -or $Approach -or $ApproachBaseline -or $StartRecovery -or $Pillik -or $PillikBaseline -or $EntryBaseline -or $Baseline)){throw 'Choose only the Magnar reset native scope'}
if($ReofferBaseline){$Reoffer=$true}
if($Reoffer -and ($Approach -or $ApproachBaseline -or $StartRecovery -or $Pillik -or $PillikBaseline -or $EntryBaseline -or $Baseline)){throw 'Choose only the reoffer native scope'}
if($Reoffer){$StartRecovery=$true}
if($ApproachBaseline){$Approach=$true}
if($Approach -and ($StartRecovery -or $Pillik -or $PillikBaseline -or $EntryBaseline -or $Baseline)){throw 'Choose only the approach native scope'}
if($Approach){$StartRecovery=$true}
if($ResetRecovery){$StartRecovery=$true}
if($ChamberRecovery){$StartRecovery=$true}
if($Barrier){$StartRecovery=$true}
if($ColdRoute){$StartRecovery=$true}
if($Replacement){$StartRecovery=$true}
if($Cursor){$StartRecovery=$true}
if($PrimaryApproach){$StartRecovery=$true}
if($CombatReplacement){$StartRecovery=$true}
if($Recreation){$StartRecovery=$true}
if($DuelRecreation){$StartRecovery=$true}
if($EntryBaseline){$Pillik=$true}
if($EntryBaseline -and $PillikBaseline){throw 'Choose one pinned Pillik counterfactual'}
if($PillikBaseline){$Pillik=$true}
if($StartRecovery -and $Pillik){throw "Choose start recovery or the earlier Pillik handoff"}
if($Baseline -and ($StartRecovery -or $Pillik)){throw "Pinned counterfactual is separate from current start recovery"}
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$family=if($AdditionalProbe){$AdditionalProbe.ToLowerInvariant()}elseif($VehicleAuraCapacity){"vehicle-aura-capacity-native"}elseif($SharasdalLadyCarry){"sharasdal-lady-carry-native"}elseif($SharasdalWaveMotion){"sharasdal-wave-motion-native"}elseif($SharasdalWaveContact){"sharasdal-wave-contact-native"}elseif($SharasdalWave){"sharasdal-wave-native"}elseif($DevourerOutro){"devourer-outro-native"}elseif($DevourerGraveyard){"devourer-graveyard-native"}elseif($KingslayersGraveyard){"kingslayers-graveyard-native"}elseif($DreadbladesGraveyard){"dreadblades-graveyard-native"}elseif($SharasdalEntrance){"sharasdal-entrance-native"}elseif($SharasdalGraveyard){"sharasdal-graveyard-native"}elseif($HolyGraveyard){"holy-graveyard-native"}elseif($ShadowGraveyard){"shadow-graveyard-native"}elseif($ArmsGraveyard){"arms-graveyard-native"}elseif($HodirGraveyard){"hodir-graveyard-native"}elseif($HodirLanding){"hodir-landing-native"}elseif($BloodGraveyard){"blood-graveyard-native"}elseif($MarksmanGraveyard){"marksmanship-graveyard-native"}elseif($TuureGraveyard){"tuure-graveyard-native"}elseif($TuureLegacy){"tuure-legacy-login-native"}elseif($TuureEntrance){"tuure-entrance-native"}elseif($BloodEntrance){"blood-entrance-native"}elseif($MarksmanEarned){"marksmanship-earned-native"}elseif($MarksmanRecovery){"marksmanship-login-recovery-native"}elseif($MarksmanshipEntrance){"marksmanship-entrance-native"}elseif($HunterRecovery){"survival-login-recovery-native"}elseif($SurvivalEntrance){"survival-entrance-native"}elseif($SurvivalGraveyard){"survival-graveyard-native"}elseif($FuryGraveyard){"warswords-graveyard-native"}elseif($WarriorRecovery){"warrior-login-recovery-native"}elseif($WarriorLogin){"warrior-scenario-resume-native"}elseif($LoginRecovery){"magnar-login-native"}elseif($DuelRecreation){"magnar-duel-recreation-native"}elseif($Recreation){"magnar-recreation-native"}elseif($CombatReplacement){"magnar-combat-native"}elseif($PrimaryApproach){"hruthnir-primary-native"}elseif($Cursor){"hruthnir-cursor-native"}elseif($Replacement){"magnar-replacement-native"}elseif($ColdRoute){"hruthnir-cold-route-native"}elseif($Barrier){"hruthnir-barrier-native"}elseif($ChamberRecovery){"hruthnir-chamber-native"}elseif($ResetRecovery){"magnar-reset-native"}elseif($Reoffer){"hruthnir-reoffer-native"}elseif($Approach){"hruthnir-approach-native"}elseif($Pillik){"hruthnir-pillik-native"}elseif($StartRecovery -or $Pillik){"hruthnir-start-native"}else{"magnar-readiness-native"}
$probe=if($VehicleAuraCapacity){"VehicleAuraCapacityNative"}elseif($SharasdalLadyCarry){"SharasdalLadyCarryNative"}elseif($SharasdalWaveMotion){"SharasdalWaveMotionNative"}elseif($SharasdalWaveContact){"SharasdalWaveContactNative"}elseif($SharasdalWave){"SharasdalWaveNative"}elseif($DevourerOutro){"DevourerOutroNative"}elseif($DevourerGraveyard){"DevourerGraveyardNative"}elseif($KingslayersGraveyard){"KingslayersGraveyardNative"}elseif($DreadbladesGraveyard){"DreadbladesGraveyardNative"}elseif($SharasdalEntrance){"SharasdalLegacyLoginNative"}elseif($SharasdalGraveyard){"SharasdalGraveyardNative"}elseif($HolyGraveyard){"HolyGraveyardNative"}elseif($ShadowGraveyard){"ShadowGraveyardNative"}elseif($ArmsGraveyard){"ArmsGraveyardNative"}elseif($HodirGraveyard){"HodirGraveyardNative"}elseif($HodirLanding){"HodirLandingNative"}elseif($BloodGraveyard){"BloodGraveyardNative"}elseif($MarksmanGraveyard){"MarksmanshipGraveyardNative"}elseif($TuureGraveyard){"TuureGraveyardNative"}elseif($TuureLegacy){"TuureLegacyLoginNative"}elseif($TuureEntrance){"TuureEntranceNative"}elseif($BloodEntrance){"BloodEntranceNative"}elseif($MarksmanEarned){"MarksmanshipEarnedReturnNative"}elseif($MarksmanRecovery){"MarksmanshipLoginRecoveryNative"}elseif($MarksmanshipEntrance){"MarksmanshipEntranceNative"}elseif($HunterRecovery){"SurvivalLoginRecoveryNative"}elseif($SurvivalEntrance){"SurvivalEntranceNative"}elseif($SurvivalGraveyard){"SurvivalGraveyardNative"}elseif($FuryGraveyard){"WarswordsGraveyardNative"}elseif($WarriorRecovery){"WarriorLoginRecoveryNative"}elseif($WarriorLogin){"WarriorScenarioResumeNative"}elseif($LoginRecovery){"MagnarLoginNative"}elseif($DuelRecreation){"MagnarDuelRecreationNative"}elseif($Recreation){"MagnarRecreationNative"}elseif($CombatReplacement){"MagnarCombatReplacementNative"}elseif($PrimaryApproach){"HruthnirPrimaryApproachNative"}elseif($Cursor){"HruthnirCursorNative"}elseif($Replacement){"MagnarReplacementNative"}elseif($ColdRoute){"HruthnirColdRouteNative"}elseif($Barrier){"HruthnirBarrierNative"}elseif($ChamberRecovery){"HruthnirChamberNative"}elseif($ResetRecovery){"MagnarResetNative"}elseif($Reoffer){"HruthnirReofferNative"}elseif($Approach){"HruthnirApproachNative"}elseif($Pillik){"HruthnirPillikNative"}elseif($StartRecovery -or $Pillik){"HruthnirStartNative"}else{"MagnarReadinessNative"}
$out="$repo/build-extractors/runtime-test/$family/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force "$out/logs"|Out-Null
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing actual $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
$source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp" -Raw
$ai=Block $source 'struct npc_sister_althea_ebonlockeAI : ScriptedAI'
foreach($pair in @(@('AltheaVictory','Player* GetVictoryPlayer('),@('Althea','void JustDied(Unit* who) override'))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $ai $pair[1]))}
[IO.File]::WriteAllText("$out/AltheaActor.inc",(Block $source 'class AltheaDeathConversation final : public Conversation')+';')
$old=(& git -C $repo show 'eb96dd4e6:src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned nearest-player baseline missing'}
# Shared bootstrap declares these pinned victory prerequisite helpers. They
# must compile, but this finale probe never invokes the old door/death hooks.
[IO.File]::WriteAllText("$out/AltheaDoor.inc",(Block (Block $old 'struct go_kingslayers_doorAI : public GameObjectAI') 'bool GossipHello('))
[IO.File]::WriteAllText("$out/AltheaBeforeCredit.inc",(Block (Block $old 'struct npc_sister_althea_ebonlockeAI : ScriptedAI') 'void JustDied(Unit* who) override'))
$quest=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
$query=[regex]::Match($quest,'(?s)void QuestDataStoreMgr::LoadQuests\(\).*?WorldDatabase.Query\(("SELECT ".*?" FROM quest_template")\);').Groups[1].Value
if(!$query){throw 'Actual native quest query missing'}
$questIds=if($SurvivalGraveyard){'40710,42504,42627,40043,39191,41105,40385'}elseif($WarriorLogin){'40710,42504,42627,40043,39191,41105'}else{'40710,42504,42627,40043,39191'}
if($HolyGraveyard){$questIds+=',42120'}
if($DreadbladesGraveyard){$questIds+=',40849'}
if($DevourerGraveyard -or $DevourerOutro){$questIds+=',41924'}
if($SharasdalGraveyard -or $SharasdalEntrance -or $SharasdalWave -or $SharasdalWaveContact -or $SharasdalWaveMotion -or $SharasdalLadyCarry){$questIds+=',40341'}
if($HodirLanding -or $HodirGraveyard){$questIds+=',42974'}
if($MarksmanshipEntrance -or $MarksmanRecovery -or $MarksmanGraveyard){$questIds+=',40419'}
if($BloodEntrance -or $BloodGraveyard){$questIds+=',40740'}
if($TuureEntrance -or $TuureGraveyard){$questIds+=',41993'}
if($AdditionalProbe -eq 'FourteenthTenNative'){$questIds+=',46110,46199,46182,47430'}
if($AdditionalProbe -eq 'FifteenthTenNative'){$questIds+=',45856'}
[IO.File]::WriteAllText("$out/NativeAltheaQuest.inc",$query+' " WHERE ID IN('+$questIds+')"')
$configPath="$repo/build-extractors/bin/Release/worldserver.conf";$hash=(Get-FileHash $configPath).Hash;$config=Get-Content $configPath -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'));[IO.File]::WriteAllText("$out/worldserver.conf",$config)
[IO.File]::WriteAllText("$out/AltheaIntro.inc",(Block $source 'class npc_sister_althea_ebonlocke : public CreatureScript')+';')
$instanceSource=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/instance_kingslayers.cpp" -Raw
if($KingslayersGraveyardBaseline){$instanceSource=(& git -C $repo show '148c610e4:src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/instance_kingslayers.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned Kingslayers instance absent'}}
[IO.File]::WriteAllText("$out/KingslayersInstance.inc",(Block $instanceSource 'class instance_kingslayers : public InstanceMapScript')+';')
$instancePath='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/instance_silver_hand.cpp'
$instance=Get-Content "$repo/$instancePath" -Raw
if(!$instance){throw 'Actual/pinned shared instance source missing'}
[IO.File]::WriteAllText("$out/SilverHandInstance.inc",(Block $instance 'class instance_silver_hand : public InstanceMapScript')+';')
$path='src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/EarthWarder.cpp'
$resetSource=if($ResetBaseline){(& git -C $repo show "4b541a943:$path") -join "`n"}else{$null}
$source=if($ReofferBaseline){(& git -C $repo show "e9ab2dd82:$path") -join "`n"}elseif($ApproachBaseline){(& git -C $repo show "58134ab60:$path") -join "`n"}elseif($EntryBaseline){(& git -C $repo show "7dc957758:$path") -join "`n"}elseif($PillikBaseline){(& git -C $repo show "cb1300a9f:$path") -join "`n"}elseif($Baseline){(& git -C $repo show "fb01c9a6a:$path") -join "`n"}else{Get-Content "$repo/$path" -Raw};if(!$source){throw 'Actual Magnar source missing'}
if($ResetBaseline){if(!$resetSource){throw 'Pinned Magnar reset source missing'};$source=$resetSource}
if($ReplacementBaseline){$source=(& git -C $repo show "7efbe0828:$path") -join "`n";if(!$source){throw 'Pinned Magnar AI replacement source missing'}}
if($ColdRouteBaseline){$source=(& git -C $repo show "05c9445bd:$path") -join "`n";if(!$source){throw 'Pinned cold route source missing'}}
if($ChamberBaseline){$source=(& git -C $repo show "827bb28aa:$path") -join "`n";if(!$source){throw 'Pinned fresh chamber source missing'}}
if($CursorBaseline){$source=(& git -C $repo show "a0f4cc39b:$path") -join "`n";if(!$source){throw 'Pinned cursor source missing'}}
if($PrimaryBaseline){$source=(& git -C $repo show "83a8cb6e3:$path") -join "`n";if(!$source){throw "Pinned original approach source missing"}}
if($CombatBaseline){$source=(& git -C $repo show "26fd47707:$path") -join "`n";if(!$source){throw "Pinned combat replacement source missing"}}
if($RecreationBaseline){$source=(& git -C $repo show "8ac9eec48:$path") -join "`n";if(!$source){throw "Pinned recreation source missing"}}
if($DuelRecreationBaseline){$source=(& git -C $repo show "89922acab:$path") -join "`n";if(!$source){throw "Pinned duel recreation source missing"}}
if($DuelDeferredBaseline){$removed=Block $source "if (first_phase && MagnarCombat::NeedsReset(";$source=$source.Replace($removed,"")}
[IO.File]::WriteAllText("$out/MagnarNative.inc",(Block $source 'Position const trashPos[12]')+';'+(Block $source 'class npc_magnar_icebreaker : public CreatureScript')+';'+$(if($source.Contains('void MagnarCombat::RetirePresentation(')){Block $source 'void MagnarCombat::RetirePresentation('}else{''}))
if($StartRecovery -or $Pillik){
 $resetBridge=if($source.Contains('void MagnarStart::ResetOffer(')){Block $source 'void MagnarStart::ResetOffer('}else{''}
 [IO.File]::WriteAllText("$out/HruthnirNative.inc",(Block $source 'namespace HruthnirGossip')+(Block $source 'class npc_hrutnir : public CreatureScript')+';'+(Block $source 'void MagnarStart::Rejoin(')+$resetBridge)
 $shared=if($PillikBaseline){(& git -C $repo show "cb1300a9f:src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/instance_shields_rest.cpp") -join "`n"}else{Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/instance_shields_rest.cpp" -Raw}
 if($ChamberBaseline){$shared=(& git -C $repo show '827bb28aa:src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/instance_shields_rest.cpp') -join "`n"}
 if($BarrierBaseline){$shared=(& git -C $repo show '73b0da45c:src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/instance_shields_rest.cpp') -join "`n"}
 if($CursorBaseline){$shared=(& git -C $repo show 'a0f4cc39b:src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/instance_shields_rest.cpp') -join "`n"}
 if($PrimaryBaseline){$shared=(& git -C $repo show "83a8cb6e3:src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/instance_shields_rest.cpp") -join "`n"}
 if($CombatBaseline){$shared=(& git -C $repo show "26fd47707:src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/instance_shields_rest.cpp") -join "`n"}
 if($RecreationBaseline){$shared=(& git -C $repo show '8ac9eec48:src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/instance_shields_rest.cpp') -join "`n"}
 if($DuelRecreationBaseline){$shared=(& git -C $repo show '89922acab:src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/instance_shields_rest.cpp') -join "`n"}
 $combatBridge=''
 foreach($name in @('void MagnarCombat::Reset(','void MagnarCombat::Bind(','void MagnarCombat::Remove(','MagnarCombat::State* MagnarCombat::Find(','void MagnarCombat::Remember(','bool MagnarCombat::Restore(','bool MagnarCombat::Rebind(','bool MagnarCombat::NeedsReset(','bool MagnarStart::ConsumeRecreation(')){if($shared.Contains($name)){$combatBridge+=Block $shared $name}}
 [IO.File]::WriteAllText("$out/MagnarCombatBridge.inc",$combatBridge.Replace('instance_shields_rest::','ShieldsFixture::instance_shields_rest::'))
 [IO.File]::WriteAllText("$out/ShieldsNative.inc",(Block $shared 'class instance_shields_rest : public InstanceMapScript')+';')
}
$previous=$env:LEGION_DIALOGUE_CONFIG;$previousCompiler=$env:CL;$previousLink=$env:_LINK_
$inputs=@("$repo/tools/tests/SlaghammerCompanionNative.cpp","$repo/src/server/game/Conditions/ConditionMgr.cpp","$repo/src/server/game/Conditions/ConditionMgr.h","$repo/src/server/game/Entities/Object/Object.h","$repo/src/server/game/Entities/Object/Object.cpp","$repo/src/server/game/Maps/Map.cpp","$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp","$repo/tools/tests/AltheaIntroNative.cpp","$repo/tools/tests/MagnarReadinessNative.cpp","$repo/src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/EarthWarder.cpp","$repo/src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/instance_silver_hand.cpp","$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/instance_kingslayers.cpp","$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/KingslayersGuardProgress.h","$repo/src/server/game/Entities/Player/Player.cpp","$repo/src/server/game/Spells/SpellEffects.cpp","$repo/build-extractors/src/server/game/Release/game.lib","$repo/build-extractors/src/server/scripts/Release/scripts.lib")
# Reusable MSBuild workers can remain idle after completion. Certify the actual
# source and final-link library bytes before/after the sequential native run.
$inputs+=@("$repo/src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/magnar_start.h","$repo/src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/hruthnir_pillik.h")
if($StartRecovery -or $Pillik){$inputs+=@("$repo/tools/tests/HruthnirStartNative.cpp","$repo/src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/instance_shields_rest.cpp","$repo/src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/shields_rest.h","$repo/src/server/game/Entities/Object/QuestObjectScope.h")}
if($Reoffer){$inputs+=@("$repo/tools/tests/HruthnirReofferNative.cpp")}
if($ResetRecovery){$inputs+=@("$repo/tools/tests/MagnarResetNative.cpp")}
if($ChamberRecovery){$inputs+=@("$repo/tools/tests/HruthnirChamberNative.cpp")}
if($Replacement){$inputs+=@("$repo/tools/tests/MagnarReplacementNative.cpp","$repo/src/server/game/Entities/Creature/Creature.cpp")}
if($DuelRecreation){$inputs+=@("$repo/tools/tests/MagnarDuelRecreationNative.cpp","$repo/tools/tests/MagnarRecreationNative.cpp","$repo/src/server/game/Entities/Creature/Creature.cpp")}
if($Recreation){$inputs+=@("$repo/tools/tests/MagnarRecreationNative.cpp","$repo/src/server/game/Entities/Creature/Creature.cpp","$repo/src/server/game/Globals/ObjectMgr.cpp")}
if($CombatReplacement){$inputs+=@("$repo/tools/tests/MagnarCombatReplacementNative.cpp","$repo/src/server/game/Entities/Creature/Creature.cpp")}
if($PrimaryApproach){$inputs+=@("$repo/tools/tests/HruthnirPrimaryApproachNative.cpp","$repo/tools/tests/HruthnirCursorNative.cpp","$repo/src/server/game/Entities/Creature/Creature.cpp","$repo/src/server/game/Movement/PathGenerator.cpp")}
if($Cursor){$inputs+=@("$repo/tools/tests/HruthnirCursorNative.cpp","$repo/src/server/game/Entities/Creature/Creature.cpp","$repo/src/server/game/Movement/PathGenerator.cpp")}
if($ColdRoute){$inputs+=@("$repo/tools/tests/HruthnirColdRouteNative.cpp","$repo/src/server/game/Movement/PathGenerator.cpp")}
if($Barrier){$inputs+=@("$repo/tools/tests/HruthnirBarrierNative.cpp","$repo/src/server/game/Entities/GameObject/GameObject.cpp","$repo/src/common/collision/Models/GameObjectModel.cpp")}
if($Approach){$inputs+=@("$repo/tools/tests/HruthnirApproachNative.cpp","$repo/src/server/game/Movement/MovementGenerators/WaypointMovementGenerator.h")}
if($Pillik){$inputs+=@("$repo/tools/tests/HruthnirPillikNative.cpp")}
if($StartRecovery -or $Pillik){$inputs+=@("$repo/src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/hruthnir_pillik.h","$out/HruthnirNative.inc","$out/ShieldsNative.inc","$out/MagnarCombatBridge.inc")}
$inputs+=@("$out/MagnarNative.inc")
$inputs+=@("$repo/src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/magnar_combat.h")
if($WarriorLogin){$inputs+=@("$repo/tools/tests/WarriorScenarioResumeNative.cpp")}
if($LoginRecovery){$inputs+=@("$repo/tools/tests/MagnarLoginNative.cpp","$repo/src/server/game/Entities/Player/PlayerScenario.cpp","$repo/src/server/game/DungeonFinding/LFGMgr.cpp")}
if($LoginRecovery){
 $migration="$repo/sql/pending/world/2026_11_07_00_earthwarder_resume_entrance.sql"
 if(!(Test-Path $migration)){$migration="$repo/sql/updates/world/2026_11_07_00_earthwarder_resume_entrance.sql"}
 [IO.File]::WriteAllText("$out/MagnarLoginMigration.inc",'R"MAGNAR_SQL('+[IO.File]::ReadAllText($migration)+')MAGNAR_SQL"')
 $inputs+=@($migration,"$out/MagnarLoginMigration.inc")
}
if($WarriorRecovery){
 $earlySource=if($MarksmanRecoveryBaseline){(& git -C $repo show '387b8a385:src/server/game/Entities/Player/Player.cpp') -join "`n"}elseif($HunterRecoveryBaseline){(& git -C $repo show '0af584162:src/server/game/Entities/Player/Player.cpp') -join "`n"}elseif($WarriorRecoveryBaseline){(& git -C $repo show '5ec346c8e:src/server/game/Entities/Player/Player.cpp') -join "`n"}else{Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw}
 $a=$earlySource.IndexOf('    // Restore before the stale-transport fallback');$b=$earlySource.IndexOf('    if(m_atLoginFlags & AT_LOGIN_UNLOCK)',$a)
 if($a -lt 0 -or $b -le $a){throw 'Native early capsule missing'}
 if($MarksmanRecoveryBaseline){
  $pinned=(& git -C $repo show '387b8a385:src/server/game/Scenarios/AcquisitionScenarioRoute.h') -join "`n";if($LASTEXITCODE){throw 'Pinned route dependency missing'}
  [IO.File]::WriteAllText("$out/PinnedAcquisitionRoute.inc",$pinned.Replace('ACQUISITION_SCENARIO_ROUTE_H','PINNED_ACQUISITION_SCENARIO_ROUTE_H').Replace('namespace AcquisitionScenarioRoute','namespace PinnedAcquisitionScenarioRoute'))
  $inputs+="$out/PinnedAcquisitionRoute.inc"
 }
 $early=$earlySource.Substring($a,$b-$a);if($MarksmanRecoveryBaseline){$early=$early.Replace('AcquisitionScenarioRoute::','PinnedAcquisitionScenarioRoute::')};if($WarriorRecoveryBaseline){$early='bool warriorQuestStatusLoaded=false;'+"`n"+$early}
 [IO.File]::WriteAllText("$out/WarriorEarlyNative.inc",$early)
 $a=$earlySource.IndexOf('    if (!scenarioQuestStatusLoaded)');$b=$earlySource.IndexOf('    _LoadQuestStatusObjectives(',$a)
 [IO.File]::WriteAllText("$out/WarriorNormalQuestNative.inc",$earlySource.Substring($a,$b-$a))
 $inputs+=@("$repo/tools/tests/WarriorLoginRecoveryNative.cpp","$repo/src/server/game/Entities/Player/Player.cpp","$repo/src/server/game/Scenarios/WarriorScenarioRoute.h","$repo/src/server/game/Scenarios/AcquisitionScenarioRoute.h","$out/WarriorEarlyNative.inc","$out/WarriorNormalQuestNative.inc")
}
if($TuureGraveyard){
 $path="$repo/src/server/scripts/Scenario/Artifacts/Priest/Tuure/instance_tuure.cpp"
 $text=if($HodirGraveyardBaseline){(& git -C $repo show 'd2f2840b2:src/server/scripts/Scenario/Artifacts/Priest/Tuure/instance_tuure.cpp') -join "`n"}elseif($BloodGraveyardBaseline){(& git -C $repo show '9d0d0d1bb:src/server/scripts/Scenario/Artifacts/Priest/Tuure/instance_tuure.cpp') -join "`n"}elseif($MarksmanGraveyardBaseline){(& git -C $repo show 'f3c39b87a:src/server/scripts/Scenario/Artifacts/Priest/Tuure/instance_tuure.cpp') -join "`n"}elseif($TuureGraveyardBaseline){(& git -C $repo show '562084277:src/server/scripts/Scenario/Artifacts/Priest/Tuure/instance_tuure.cpp') -join "`n"}else{Get-Content $path -Raw}
 if(!$text){throw 'Tuure instance source absent'}
 [IO.File]::WriteAllText("$out/TuureGraveyardInstance.inc",(Block $text 'class instance_tuure : public InstanceMapScript')+';')
 $inputs+=@($path,"$repo/src/server/scripts/Scenario/Artifacts/Priest/Tuure/Tuure.cpp","$repo/src/server/scripts/Scenario/Artifacts/Priest/Tuure/Tuure.h","$repo/tools/tests/TuureGraveyardNative.cpp","$repo/tools/tests/MarksmanshipGraveyardNative.cpp","$repo/tools/tests/BloodGraveyardNative.cpp","$repo/tools/tests/HodirGraveyardNative.cpp","$out/TuureGraveyardInstance.inc")
}
if($FuryGraveyard){
 $instancePath="$repo/src/server/scripts/Scenario/Artifacts/Warrior/Warswords/instance_warswords.cpp"
 $instance=if($SurvivalGraveyardBaseline){(& git -C $repo show '5d3d9312b:src/server/scripts/Scenario/Artifacts/Warrior/Warswords/instance_warswords.cpp') -join "`n"}elseif($FuryGraveyardBaseline){(& git -C $repo show 'c252ae5ee:src/server/scripts/Scenario/Artifacts/Warrior/Warswords/instance_warswords.cpp') -join "`n"}else{Get-Content $instancePath -Raw}
 if(!$instance){throw 'Actual/pinned Fury graveyard instance missing'}
 [IO.File]::WriteAllText("$out/WarswordsGraveyardInstance.inc",$instance)
 $inputs+=@("$repo/tools/tests/WarswordsGraveyardNative.cpp","$repo/tools/tests/SurvivalGraveyardNative.cpp",$instancePath,"$out/WarswordsGraveyardInstance.inc","$repo/src/server/scripts/Scenario/Artifacts/Warrior/Warswords/warswords_arrival.h","$repo/src/server/scripts/Scenario/Artifacts/Warrior/Warswords/warswords_waves.h","$repo/src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/instance_silver_hand.cpp")
}
if($SurvivalEntrance){
 $migration="$repo/sql/pending/world/2026_11_07_01_survival_resume_entrances.sql"
 if(!(Test-Path $migration)){$migration="$repo/sql/updates/world/2026_11_07_01_survival_resume_entrances.sql"}
 [IO.File]::WriteAllText("$out/SurvivalEntranceMigration.inc",'R"HUNTER_SQL('+[IO.File]::ReadAllText($migration)+')HUNTER_SQL"')
 $inputs+=@("$repo/tools/tests/SurvivalEntranceNative.cpp",$migration,"$out/SurvivalEntranceMigration.inc")
}
if($TuureLegacy -or $TuureEntranceBaseline){
 $inputs+=@("$repo/tools/tests/TuureLegacyLoginNative.cpp","$repo/tools/tests/TuurePinnedPrepareNative.h","$repo/src/server/game/Entities/Player/PlayerScenario.cpp")
 $pinned=(& git -C $repo show 'fdb24afb8:src/server/game/Entities/Player/PlayerScenario.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned Tuure Prepare source unavailable'}
 $a=$pinned.IndexOf('namespace');$b=$pinned.IndexOf('void Player::PrepareScenarioLogin(',$a);[IO.File]::WriteAllText("$out/TuurePinnedNamespace.inc",$pinned.Substring($a,$b-$a))
 $body=Block $pinned 'void Player::PrepareScenarioLogin('; $a=$body.IndexOf('{')+1;$body=$body.Substring($a,$body.LastIndexOf('}')-$a).Replace('ResumeDungeon(this,','ResumeDungeon(&player,')
 [IO.File]::WriteAllText("$out/TuurePinnedPrepare.inc",$body);$inputs+=@("$out/TuurePinnedNamespace.inc","$out/TuurePinnedPrepare.inc")
}
if($TuureEntrance){
 $migration="$repo/sql/pending/world/2026_11_08_02_tuure_resume_entrance.sql";if(!(Test-Path $migration)){$migration="$repo/sql/updates/world/2026_11_08_02_tuure_resume_entrance.sql"}
 [IO.File]::WriteAllText("$out/TuureEntranceMigration.inc",'R"TUURE_SQL('+[IO.File]::ReadAllText($migration)+')TUURE_SQL"')
 $inputs+=@("$repo/tools/tests/TuureEntranceNative.cpp",$migration,"$out/TuureEntranceMigration.inc")
}
if($BloodEntrance){
 $migration="$repo/sql/pending/world/2026_11_08_01_blood_resume_entrance.sql";if(!(Test-Path $migration)){$migration="$repo/sql/updates/world/2026_11_08_01_blood_resume_entrance.sql"}
 [IO.File]::WriteAllText("$out/BloodEntranceMigration.inc",'R"BLOOD_SQL('+[IO.File]::ReadAllText($migration)+')BLOOD_SQL"')
 $inputs+=@("$repo/tools/tests/BloodEntranceNative.cpp",$migration,"$out/BloodEntranceMigration.inc")
}
if($MarksmanEarned){
 $path="$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp";$text=Get-Content $path -Raw
 [IO.File]::WriteAllText("$out/EarnedConditionNative.inc",(Block $text 'class condition_marksmanship_recovered_arrival : public ConditionScript')+';')
 $inputs+=@($path,"$repo/src/server/game/Entities/Player/PlayerScenario.cpp","$repo/src/server/game/Entities/Player/Player.h","$repo/tools/tests/MarksmanshipEarnedReturnNative.cpp","$out/EarnedConditionNative.inc")
}

if($MarksmanRecovery){
 $a=$earlySource.IndexOf('    // A new entrance-only Hunter recovery');$b=$earlySource.IndexOf('    if (!map)',$a);if($a -lt 0 -or $b -le $a){throw 'Actual selected-map guard missing'}
 [IO.File]::WriteAllText("$out/MarksmanshipBindingNative.inc",$earlySource.Substring($a,$b-$a))
 $inputs+=@("$repo/tools/tests/MarksmanshipLoginRecoveryNative.cpp","$out/MarksmanshipBindingNative.inc")
}

if($MarksmanshipEntrance){
 $migration="$repo/sql/pending/world/2026_11_07_02_marksmanship_resume_entrance.sql";if(!(Test-Path $migration)){$migration="$repo/sql/updates/world/2026_11_07_02_marksmanship_resume_entrance.sql"}
 [IO.File]::WriteAllText("$out/MarksmanshipEntranceMigration.inc",'R"MARK_SQL('+[IO.File]::ReadAllText($migration)+')MARK_SQL"')
 $inputs+=@("$repo/tools/tests/MarksmanshipEntranceNative.cpp",$migration,"$out/MarksmanshipEntranceMigration.inc")
}

if($HunterRecovery){
 if($HunterRecoveryBaseline){$binding='/* pinned loader lacks selected-live-binding guard */'}else{$a=$earlySource.IndexOf('    // A new entrance-only Hunter recovery');$b=$earlySource.IndexOf('    if (!map)',$a);if($a -lt 0 -or $b -le $a){throw 'Actual selected-map guard missing'};$binding=$earlySource.Substring($a,$b-$a)}
 [IO.File]::WriteAllText("$out/SurvivalBindingNative.inc",$binding)
 $scenarioSource=Get-Content "$repo/src/server/game/Entities/Player/PlayerScenario.cpp" -Raw
 if($HunterRecoveryBaseline){$exit='/* pinned handler has no owned-claim exit */'}else{$exit=Block $scenarioSource 'bool Player::ExitCompletedTalonclawLogin()';$a=$exit.IndexOf('{')+1;$exit=$exit.Substring($a,$exit.LastIndexOf('}')-$a)}
 [IO.File]::WriteAllText("$out/TalonclawLoginExitNative.inc",$exit)
 $handler=if($HunterRecoveryBaseline){(& git -C $repo show '0af584162:src/server/game/Handlers/CharacterHandler.cpp') -join "`n"}else{Get-Content "$repo/src/server/game/Handlers/CharacterHandler.cpp" -Raw}
 $condition=[regex]::Match($handler,'(?m)^\s*if \((!validLoginMap .*?)\)\s*$').Groups[1].Value
 if(!$condition){throw 'Actual invalid-login exit chain missing'}
 [IO.File]::WriteAllText("$out/HunterLoginCaller.inc",'return '+$condition+';')
 $inputs+=@("$repo/tools/tests/SurvivalLoginRecoveryNative.cpp","$repo/src/server/game/Handlers/CharacterHandler.cpp","$out/SurvivalBindingNative.inc","$out/TalonclawLoginExitNative.inc","$out/HunterLoginCaller.inc")
}
if($HodirLanding){
 $path="$repo/src/server/game/AI/SmartScripts/SmartAI.cpp"
 [IO.File]::WriteAllText("$out/HodirSmartEventNative.inc",(Block (Get-Content $path -Raw) 'class SmartEventObject : public EventObjectScript')+';')
 $migration="$repo/sql/pending/world/2026_11_08_03_hodir_landing_admission.sql";if(!(Test-Path $migration)){$migration="$repo/sql/updates/world/2026_11_08_03_hodir_landing_admission.sql"}
 [IO.File]::WriteAllText("$out/HodirLandingMigration.inc",'R"SQL('+[IO.File]::ReadAllText($migration)+')SQL"')
 $inputs+=@($path,$migration,"$out/HodirSmartEventNative.inc","$out/HodirLandingMigration.inc","$repo/tools/tests/HodirLandingNative.cpp","$repo/src/server/game/AI/SmartScripts/SmartScript.cpp","$repo/src/server/game/Conditions/ConditionMgr.cpp","$repo/src/server/game/Entities/EventObject/EventObject.cpp","$repo/src/server/game/Entities/EventObject/EventObject.h")
}
if($ArmsGraveyard){
 $path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/instance_silver_hand.cpp'
 $text=if($ArmsGraveyardBaseline){(& git -C $repo show "96ca7646c:$path") -join "`n"}else{Get-Content "$repo/$path" -Raw}
 [IO.File]::WriteAllText("$out/ArmsGraveyardInstance.inc",(Block $text 'class instance_silver_hand : public InstanceMapScript')+';')
 $inputs+=@("$repo/$path","$repo/tools/tests/ArmsGraveyardNative.cpp","$out/ArmsGraveyardInstance.inc")
}
if($ShadowGraveyard){
 $path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/instance_silver_hand.cpp'
 $text=if($ShadowGraveyardBaseline){(& git -C $repo show "41e2b7743:$path") -join "`n"}else{Get-Content "$repo/$path" -Raw}
 [IO.File]::WriteAllText("$out/ShadowGraveyardInstance.inc",(Block $text 'class instance_silver_hand : public InstanceMapScript')+';')
 $inputs+=@("$repo/$path","$repo/tools/tests/ShadowGraveyardNative.cpp","$out/ShadowGraveyardInstance.inc")
}
if($HolyGraveyard){
 $path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/instance_silver_hand.cpp'
 $text=if($HolyGraveyardBaseline){(& git -C $repo show "2bd533cfd:$path") -join "`n"}else{Get-Content "$repo/$path" -Raw}
 [IO.File]::WriteAllText("$out/HolyGraveyardInstance.inc",(Block $text 'class instance_silver_hand : public InstanceMapScript')+';')
 $inputs+=@("$repo/$path","$repo/tools/tests/HolyGraveyardNative.cpp","$out/HolyGraveyardInstance.inc")
}
if($SharasdalGraveyard){
 $path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/instance_sharasdal.cpp'
 $text=if($SharasdalGraveyardBaseline){(& git -C $repo show "d8cc8bb9e:$path") -join "`n"}else{Get-Content "$repo/$path" -Raw}
 $a=$text.IndexOf('namespace SharasdalGuard');$end=$text.IndexOf('class instance_sharasdal : public InstanceMapScript')
 [IO.File]::WriteAllText("$out/SharasdalGraveyardInstance.inc",$text.Substring($a,$end-$a)+(Block $text 'class instance_sharasdal : public InstanceMapScript')+';')
 $inputs+=@("$repo/$path","$repo/tools/tests/SharasdalGraveyardNative.cpp","$repo/src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/sharasdal_lower_ascent.h","$out/SharasdalGraveyardInstance.inc")
}
if($SharasdalWave -or $SharasdalWaveContact -or $SharasdalWaveMotion){
 $path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp'
 $text=if($SharasdalWaveBaseline){(& git -C $repo show "dbb95ce8d:$path") -join "`n"}else{Get-Content "$repo/$path" -Raw}
 if(!$text){throw 'Actual/pinned Sharasdal wave absent'}
 [IO.File]::WriteAllText("$out/SharasdalWave.inc",(Block $text 'class npc_sharasdal_waterwall :')+';')
 $inputs+=@("$repo/$path","$repo/tools/tests/SharasdalWaveNative.cpp","$out/SharasdalWave.inc","$repo/src/server/game/Entities/Unit/Unit.cpp","$repo/src/server/game/Entities/Vehicle/Vehicle.cpp","$repo/src/server/game/Entities/Creature/TemporarySummon.cpp","$repo/src/common/Utilities/EventProcessor.cpp")
}
if($VehicleAuraCapacity){$inputs+=@("$repo/tools/tests/VehicleAuraCapacityNative.cpp","$repo/src/server/game/Spells/Auras/SpellAuras.cpp","$repo/src/server/game/Spells/Auras/SpellAuraEffects.cpp","$repo/src/server/game/Entities/Vehicle/Vehicle.cpp")}
if($VehicleAuraCapacityBaseline){$pinned=(& git -C $repo show '798e39578:src/server/game/Spells/Auras/SpellAuras.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned pre-fix native aura implementation absent'};[IO.File]::WriteAllText("$out/VehicleAuraCapacityPinned.inc",$pinned);$inputs+="$out/VehicleAuraCapacityPinned.inc"}
if($SharasdalLadyCarry){
 $migration="$repo/sql/pending/world/2026_11_08_05_sharasdal_grash_carry_target.sql";if(!(Test-Path $migration)){$migration="$repo/sql/updates/world/2026_11_08_05_sharasdal_grash_carry_target.sql"}
 [IO.File]::WriteAllText("$out/SharasdalCarryMigration.inc",'R"CARRY_SQL('+[IO.File]::ReadAllText($migration)+')CARRY_SQL"')
 $inputs+=@("$repo/tools/tests/SharasdalLadyCarryNative.cpp",$migration,"$out/SharasdalCarryMigration.inc","$repo/src/server/game/Spells/Spell.cpp","$repo/src/server/game/Spells/Auras/SpellAuraEffects.cpp","$repo/src/server/game/Entities/Vehicle/Vehicle.cpp","$repo/src/server/game/Conditions/ConditionMgr.cpp")
}
if($SharasdalWaveMotion){
 [IO.File]::WriteAllText("$out/SharasdalWaveLanes.inc",(Block $text 'Position const sumpos[3] =')+';')
 $endpoint=[regex]::Matches($text,'add->GetMotionMaster\(\)->MovePoint\(1,\s*([^;]+)\);');if($endpoint.Count -ne 1){throw 'Actual authored wave endpoint changed'}
 [IO.File]::WriteAllText("$out/SharasdalWaveEnd.inc",'Position const waveEnd{'+$endpoint[0].Groups[1].Value+',0.f};')
 $inputs+=@("$repo/tools/tests/SharasdalWaveMotionNative.cpp","$out/SharasdalWaveLanes.inc","$out/SharasdalWaveEnd.inc","$repo/src/server/game/Entities/AreaTrigger/AreaTrigger.cpp","$repo/src/server/game/Entities/AreaTrigger/SharasdalWaveRide.h","$repo/src/server/game/Globals/AreaTriggerData.cpp","$repo/src/server/game/Maps/Map.cpp","$repo/src/server/game/Movement/MotionMaster.cpp","$repo/src/server/game/Movement/PathGenerator.cpp","$repo/src/server/game/Movement/MovementGenerators/PointMovementGenerator.cpp","$repo/src/server/game/Movement/Spline/MoveSpline.cpp")
}
if($SharasdalWaveContact){$inputs+=@("$repo/tools/tests/SharasdalWaveContactNative.cpp","$repo/src/server/game/Entities/AreaTrigger/AreaTrigger.cpp","$repo/src/server/game/Entities/AreaTrigger/AreaTrigger.h","$repo/src/server/game/Entities/AreaTrigger/SharasdalWaveRide.h","$repo/src/server/game/Globals/AreaTriggerData.cpp")}
if($DevourerOutro){
 $path='src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/instance_devourer.cpp'
 $text=if($DevourerOutroBaseline){(& git -C $repo show "e59f20dd3:$path") -join "`n"}else{Get-Content "$repo/$path" -Raw}
 if(!$text){throw 'Actual/pinned Devourer outro absent'}
 [IO.File]::WriteAllText("$out/DevourerOutroInstance.inc",(Block $text 'class instance_devourer : public InstanceMapScript')+';')
 $inputs+=@("$repo/$path","$repo/tools/tests/DevourerOutroNative.cpp","$out/DevourerOutroInstance.inc")
}
if($DevourerGraveyard){
 $path='src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/instance_devourer.cpp'
 $text=if($DevourerGraveyardBaseline){(& git -C $repo show "3465a1d48:$path") -join "`n"}else{Get-Content "$repo/$path" -Raw}
 [IO.File]::WriteAllText("$out/DevourerGraveyardInstance.inc",(Block $text 'class instance_devourer : public InstanceMapScript')+';')
 $inputs+=@("$repo/$path","$repo/tools/tests/DevourerGraveyardNative.cpp","$out/DevourerGraveyardInstance.inc")
}
if($KingslayersGraveyard){$inputs+=@("$repo/tools/tests/KingslayersGraveyardNative.cpp","$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/instance_kingslayers.cpp","$out/KingslayersInstance.inc")}
if($DreadbladesGraveyard){
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/instance_class_art_scenarios.cpp'
 $text=if($DreadbladesGraveyardBaseline){(& git -C $repo show "9a8f9340d:$path") -join "`n"}else{Get-Content "$repo/$path" -Raw}
 [IO.File]::WriteAllText("$out/DreadbladesGraveyardInstance.inc",(Block $text 'struct instance_dreadblades : public InstanceScript')+';')
 $inputs+=@("$repo/$path","$repo/tools/tests/DreadbladesGraveyardNative.cpp","$out/DreadbladesGraveyardInstance.inc")
}
if($SharasdalEntrance){
 $migration="$repo/sql/pending/world/2026_11_08_04_sharasdal_resume_entrance.sql";if(!(Test-Path $migration)){$migration="$repo/sql/updates/world/2026_11_08_04_sharasdal_resume_entrance.sql"}
 [IO.File]::WriteAllText("$out/SharasdalEntranceMigration.inc",'R"SHARA_SQL('+[IO.File]::ReadAllText($migration)+')SHARA_SQL"')
 $inputs+=@("$repo/tools/tests/SharasdalLegacyLoginNative.cpp","$repo/tools/tests/SharasdalEntranceNative.h",$migration,"$repo/src/server/game/Entities/Player/PlayerScenario.cpp","$out/SharasdalEntranceMigration.inc")
 if($SharasdalEntranceBaseline){
  $pinned=(& git -C $repo show '677430a07:src/server/game/Entities/Player/PlayerScenario.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned Sharasdal Prepare absent'}
  $a=$pinned.IndexOf('namespace');$b=$pinned.IndexOf('void Player::PrepareScenarioLogin(',$a);[IO.File]::WriteAllText("$out/SharasdalPinnedNamespace.inc",$pinned.Substring($a,$b-$a))
  $body=Block $pinned 'void Player::PrepareScenarioLogin('; $a=$body.IndexOf('{')+1;$body=$body.Substring($a,$body.LastIndexOf('}')-$a).Replace('ResumeDungeon(this,','ResumeDungeon(&player,').Replace('RecoverLegacyTuureEntrance(this,','RecoverLegacyTuureEntrance(&player,')
  $body=$body.Replace('auto current = GetPosition();','auto current = player.GetPosition();')
  [IO.File]::WriteAllText("$out/SharasdalPinnedPrepare.inc",$body);$inputs+=@("$repo/tools/tests/SharasdalPinnedPrepareNative.h","$out/SharasdalPinnedNamespace.inc","$out/SharasdalPinnedPrepare.inc")
 }
}
if($AdditionalProbe){if(@($PSBoundParameters.Keys|Where-Object {$_ -notin @("AdditionalProbe","AdditionalBaseline")}).Count){throw "Choose only AdditionalProbe"};$probe=$AdditionalProbe;$inputs+="$repo/tools/tests/$probe.cpp"}
if($AdditionalProbe -eq 'SevenSpellLifecycleNative'){& "$PSScriptRoot/Prepare-SevenSpellNative.ps1" -OutputDirectory $out;$inputs+=@("$out/SevenSpellScripts.inc","$repo/src/server/scripts/Spells/spell_generic.cpp","$repo/src/server/scripts/Spells/spell_item.cpp")}
if($AdditionalProbe -eq 'TenSpellLifecycleNative'){& "$PSScriptRoot/Prepare-TenSpellNative.ps1" -OutputDirectory $out;$inputs+=@("$out/TenSpellScripts.inc","$out/TenSpellBindings.inc","$repo/tools/tests/Prepare-TenSpellNative.ps1","$repo/tools/tests/TenSpellLifecycles.json","$repo/src/server/scripts/Spells/spell_generic.cpp","$repo/src/server/scripts/Spells/spell_item.cpp","$repo/src/server/scripts/Spells/spell_monk.cpp","$repo/src/server/scripts/Spells/spell_paladin.cpp")}
if($AdditionalProbe -eq 'NextTenSpellNative'){& "$PSScriptRoot/Prepare-NextTenSpellNative.ps1" -OutputDirectory $out;$inputs+=@("$out/NextTenSpellScripts.inc","$out/NextTenSpellBindings.inc","$repo/tools/tests/Prepare-NextTenSpellNative.ps1","$repo/tools/tests/NextTenSpellRepairs.json","$repo/src/server/scripts/Spells/spell_generic.cpp","$repo/src/server/scripts/Spells/spell_item.cpp","$repo/src/server/scripts/Spells/spell_mage.cpp","$repo/src/server/scripts/Spells/spell_warrior.cpp","$repo/src/server/scripts/Spells/spell_warlock.cpp")}
if($AdditionalProbe -eq 'ThirdTenSpellNative'){& "$PSScriptRoot/Prepare-ThirdTenSpellNative.ps1" -OutputDirectory $out;$inputs+=@("$out/ThirdTenSpellScripts.inc","$out/ThirdTenSpellBindings.inc","$out/ThirdTenConstants.inc","$repo/tools/tests/Prepare-ThirdTenSpellNative.ps1","$repo/tools/tests/ThirdTenSpellRepairs.json","$repo/src/server/scripts/Spells/spell_generic.cpp","$repo/src/server/scripts/Spells/spell_item.cpp","$repo/src/server/scripts/Spells/spell_priest.cpp","$repo/src/server/scripts/Spells/spell_monk.cpp","$repo/src/server/scripts/Spells/spell_dk.cpp")}

if($AdditionalProbe -eq 'HealthDeltaNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_health_delta.py" $out
 if($LASTEXITCODE){throw 'Native health baseline extraction failed'}
 $inputs+=@("$out/HealthBefore.inc","$repo/tools/tests/prepare_health_delta.py","$repo/src/server/game/Entities/Unit/Unit.cpp")
}
if($AdditionalProbe -eq 'FreyaElderHealthNative'){
 $freyaMode=if($env:CL -match 'FREYA_ELDER_HEALTH_BASELINE'){'baseline'}else{'fixed'}
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_freya_elder_health.py" $out $freyaMode
 if($LASTEXITCODE){throw 'Freya source extraction failed'}
 $inputs+=@("$out/FreyaElderHealth.inc","$repo/tools/tests/prepare_freya_elder_health.py","$repo/src/server/scripts/Northrend/Ulduar/Ulduar/boss_freya.cpp")
}
if($AdditionalProbe -eq 'TsulongTransitionNative'){
 $tsulongMode=if($env:CL -match 'TSULONG_TRANSITION_BASELINE'){'baseline'}else{'fixed'}
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_tsulong_transition.py" $out $tsulongMode
 if($LASTEXITCODE){throw 'Tsulong source extraction failed'}
 $inputs+=@("$out/TsulongTransition.inc","$repo/tools/tests/prepare_tsulong_transition.py","$repo/src/server/scripts/Pandaria/TerraceEndlessSpring/boss_tsulong.cpp")
}
if($AdditionalProbe -eq 'KarazhanRookNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_karazhan_rook.py" $out
 if($LASTEXITCODE){throw 'Chess native source extraction failed'}
 $inputs+=@("$out/ChessAI.inc","$out/ChessConstants.inc","$repo/tools/tests/prepare_karazhan_rook.py","$repo/src/server/scripts/Legion/ReturnToKarazhan/karazhan_chess.cpp")
}
if($AdditionalProbe -eq 'ThirtySixthTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_thirtysixth.py" $out
 if($LASTEXITCODE){throw 'ThirtySixth native extraction failed'}
 $inputs+=@("$repo/src/common/Common.h","$repo/tools/tests/ActorQueueLifetimeNative.h","$repo/tools/tests/LifetimeCallbackNative.h","$repo/tools/tests/prepare_lifetime_queue.py","$repo/tools/tests/LifetimeCallbackNative.h","$repo/tools/tests/prepare_lifetime_queue.py","$repo/tools/tests/LifetimeCallbackNative.h","$repo/tools/tests/prepare_lifetime_queue.py","$repo/tools/tests/LifetimeCallbackNative.h","$repo/tools/tests/prepare_lifetime_queue.py","$repo/tools/tests/prepare_actor_queue.py","$out/ThirtySixthNativeScripts.inc","$repo/tools/tests/prepare_fifteenth.py","$repo/src/server/game/Entities/Object/ObjectGuid.h","$repo/src/server/game/Entities/Pet/PetDefines.h","$repo/src/server/game/Movement/MotionMaster.h","$repo/tools/tests/prepare_thirtysixth.py","$repo/tools/tests/ThirtySixthTenRepairs.json")
 $thirtysixthPlan=Get-Content "$repo/tools/tests/ThirtySixthTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($thirtysixthPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'ThirtyFifthTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_thirtyfifth.py" $out
 if($LASTEXITCODE){throw 'ThirtyFifth native extraction failed'}
 $inputs+=@("$repo/src/common/Common.h","$repo/tools/tests/ActorQueueLifetimeNative.h","$repo/tools/tests/LifetimeCallbackNative.h","$repo/tools/tests/prepare_lifetime_queue.py","$repo/tools/tests/LifetimeCallbackNative.h","$repo/tools/tests/prepare_lifetime_queue.py","$repo/tools/tests/LifetimeCallbackNative.h","$repo/tools/tests/prepare_lifetime_queue.py","$repo/tools/tests/prepare_actor_queue.py","$out/ThirtyFifthNativeScripts.inc","$repo/tools/tests/prepare_fifteenth.py","$repo/src/server/game/Entities/Object/ObjectGuid.h","$repo/src/server/game/Entities/Pet/PetDefines.h","$repo/src/server/game/Movement/MotionMaster.h","$repo/tools/tests/prepare_thirtyfifth.py","$repo/tools/tests/ThirtyFifthTenRepairs.json")
 $thirtyfifthPlan=Get-Content "$repo/tools/tests/ThirtyFifthTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($thirtyfifthPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'ThirtyFourthTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_thirtyfourth.py" $out
 if($LASTEXITCODE){throw 'ThirtyFourth native extraction failed'}
 $inputs+=@("$repo/src/common/Common.h","$repo/tools/tests/ActorQueueLifetimeNative.h","$repo/tools/tests/LifetimeCallbackNative.h","$repo/tools/tests/prepare_lifetime_queue.py","$repo/tools/tests/LifetimeCallbackNative.h","$repo/tools/tests/prepare_lifetime_queue.py","$repo/tools/tests/prepare_actor_queue.py","$out/ThirtyFourthNativeScripts.inc","$repo/tools/tests/prepare_fifteenth.py","$repo/src/server/game/Entities/Object/ObjectGuid.h","$repo/src/server/game/Entities/Pet/PetDefines.h","$repo/src/server/game/Movement/MotionMaster.h","$repo/tools/tests/prepare_thirtyfourth.py","$repo/tools/tests/ThirtyFourthTenRepairs.json")
 $thirtyfourthPlan=Get-Content "$repo/tools/tests/ThirtyFourthTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($thirtyfourthPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'ThirtyThirdTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_thirtythird.py" $out
 if($LASTEXITCODE){throw 'ThirtyThird native extraction failed'}
 $inputs+=@("$repo/src/common/Common.h","$repo/tools/tests/ActorQueueLifetimeNative.h","$repo/tools/tests/LifetimeCallbackNative.h","$repo/tools/tests/prepare_lifetime_queue.py","$repo/tools/tests/prepare_actor_queue.py","$out/ThirtyThirdNativeScripts.inc","$repo/tools/tests/prepare_fifteenth.py","$repo/src/server/game/Entities/Object/ObjectGuid.h","$repo/src/server/game/Entities/Pet/PetDefines.h","$repo/src/server/game/Movement/MotionMaster.h","$repo/tools/tests/prepare_thirtythird.py","$repo/tools/tests/ThirtyThirdTenRepairs.json")
 $thirtythirdPlan=Get-Content "$repo/tools/tests/ThirtyThirdTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($thirtythirdPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'ThirtySecondTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_thirtysecond.py" $out
 if($LASTEXITCODE){throw 'ThirtySecond native extraction failed'}
 $inputs+=@("$repo/src/common/Common.h","$repo/tools/tests/ActorQueueLifetimeNative.h","$repo/tools/tests/prepare_actor_queue.py","$out/ThirtySecondNativeScripts.inc","$repo/tools/tests/prepare_fifteenth.py","$repo/src/server/game/Entities/Object/ObjectGuid.h","$repo/src/server/game/Entities/Pet/PetDefines.h","$repo/src/server/game/Movement/MotionMaster.h","$repo/tools/tests/prepare_thirtysecond.py","$repo/tools/tests/ThirtySecondTenRepairs.json")
 $thirtysecondPlan=Get-Content "$repo/tools/tests/ThirtySecondTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($thirtysecondPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'ThirtyFirstTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_thirtyfirst.py" $out
 if($LASTEXITCODE){throw 'ThirtyFirst native extraction failed'}
 $inputs+=@("$repo/src/common/Common.h","$repo/tools/tests/CoreDelayTalkNative.h","$repo/src/server/game/Texts/CreatureTextMgr.h","$repo/src/server/game/Texts/CreatureTextMgr.cpp","$repo/tools/tests/ActorQueueLifetimeNative.h","$repo/tools/tests/prepare_actor_queue.py","$out/ThirtyFirstNativeScripts.inc","$repo/tools/tests/prepare_fifteenth.py","$repo/src/server/game/Entities/Object/ObjectGuid.h","$repo/src/server/game/Entities/Pet/PetDefines.h","$repo/src/server/game/Movement/MotionMaster.h","$repo/tools/tests/prepare_thirtyfirst.py","$repo/tools/tests/ThirtyFirstTenRepairs.json")
 $thirtyfirstPlan=Get-Content "$repo/tools/tests/ThirtyFirstTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($thirtyfirstPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'ThirtiethTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_thirtieth.py" $out
 if($LASTEXITCODE){throw 'Thirtieth native extraction failed'}
 $inputs+=@("$repo/tools/tests/ActorQueueLifetimeNative.h","$repo/tools/tests/prepare_actor_queue.py","$out/ThirtiethNativeScripts.inc","$repo/tools/tests/prepare_fifteenth.py","$repo/src/server/game/Entities/Object/ObjectGuid.h","$repo/src/server/game/Entities/Pet/PetDefines.h","$repo/src/server/game/Movement/MotionMaster.h","$repo/tools/tests/prepare_thirtieth.py","$repo/tools/tests/ThirtiethTenRepairs.json")
 $thirtiethPlan=Get-Content "$repo/tools/tests/ThirtiethTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($thirtiethPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'TwentyNinthTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_twentyninth.py" $out
 if($LASTEXITCODE){throw 'TwentyNinth native extraction failed'}
 $inputs+=@("$repo/tools/tests/HasabelWaveFollowup.json","$out/HasabelWaveNativeScripts.inc","$repo/src/server/scripts/Legion/AntorusTheBurningThrone/boss_hasabel.cpp","$repo/tools/tests/ActorQueueLifetimeNative.h","$repo/tools/tests/prepare_actor_queue.py","$out/TwentyNinthNativeScripts.inc","$repo/tools/tests/prepare_fifteenth.py","$repo/src/server/game/Entities/Object/ObjectGuid.h","$repo/src/server/game/Entities/Pet/PetDefines.h","$repo/src/server/game/Movement/MotionMaster.h","$repo/tools/tests/prepare_twentyninth.py","$repo/tools/tests/TwentyNinthTenRepairs.json")
 $twentyninthPlan=Get-Content "$repo/tools/tests/TwentyNinthTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($twentyninthPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'TwentyEighthTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_twentyeighth.py" $out
 if($LASTEXITCODE){throw 'TwentyEighth native extraction failed'}
 $inputs+=@("$repo/tools/tests/ActorQueueLifetimeNative.h","$repo/tools/tests/prepare_actor_queue.py","$out/TwentyEighthNativeScripts.inc","$repo/tools/tests/prepare_fifteenth.py","$repo/src/server/game/Entities/Object/ObjectGuid.h","$repo/src/server/game/Entities/Pet/PetDefines.h","$repo/src/server/game/Movement/MotionMaster.h","$repo/tools/tests/prepare_twentyeighth.py","$repo/tools/tests/TwentyEighthTenRepairs.json")
 $twentyeighthPlan=Get-Content "$repo/tools/tests/TwentyEighthTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($twentyeighthPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'TwentySeventhTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_twentyseventh.py" $out
 if($LASTEXITCODE){throw 'TwentySeventh native extraction failed'}
 $inputs+=@("$repo/tools/tests/ActorQueueLifetimeNative.h","$repo/tools/tests/prepare_actor_queue.py","$out/TwentySeventhNativeScripts.inc","$repo/tools/tests/prepare_fifteenth.py","$repo/src/server/game/Entities/Object/ObjectGuid.h","$repo/src/server/game/Entities/Pet/PetDefines.h","$repo/src/server/game/Movement/MotionMaster.h","$repo/tools/tests/prepare_twentyseventh.py","$repo/tools/tests/TwentySeventhTenRepairs.json")
 $twentyseventhPlan=Get-Content "$repo/tools/tests/TwentySeventhTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($twentyseventhPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'TwentySixthTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_twentysixth.py" $out
 if($LASTEXITCODE){throw 'TwentySixth native extraction failed'}
 $inputs+=@("$out/TwentySixthNativeScripts.inc","$repo/tools/tests/prepare_fifteenth.py","$repo/src/server/game/Entities/Object/ObjectGuid.h","$repo/src/server/game/Entities/Pet/PetDefines.h","$repo/src/server/game/Movement/MotionMaster.h","$repo/tools/tests/prepare_twentysixth.py","$repo/tools/tests/TwentySixthTenRepairs.json")
 $twentysixthPlan=Get-Content "$repo/tools/tests/TwentySixthTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($twentysixthPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'TwentyFifthTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_twentyfifth.py" $out
 if($LASTEXITCODE){throw 'TwentyFifth native extraction failed'}
 $inputs+=@("$out/TwentyFifthNativeScripts.inc","$repo/tools/tests/prepare_fifteenth.py","$repo/src/server/game/Entities/Object/ObjectGuid.h","$repo/src/server/game/Entities/Pet/PetDefines.h","$repo/src/server/game/Movement/MotionMaster.h","$repo/tools/tests/prepare_twentyfifth.py","$repo/tools/tests/TwentyFifthTenRepairs.json")
 $twentyfifthPlan=Get-Content "$repo/tools/tests/TwentyFifthTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($twentyfifthPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'TwentyFourthTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_twentyfourth.py" $out
 if($LASTEXITCODE){throw 'TwentyFourth native extraction failed'}
 $inputs+=@("$out/TwentyFourthNativeScripts.inc","$repo/tools/tests/prepare_fifteenth.py","$repo/src/server/game/Entities/Object/ObjectGuid.h","$repo/src/server/game/Entities/Pet/PetDefines.h","$repo/src/server/game/Movement/MotionMaster.h","$repo/tools/tests/prepare_twentyfourth.py","$repo/tools/tests/TwentyFourthTenRepairs.json")
 $twentyfourthPlan=Get-Content "$repo/tools/tests/TwentyFourthTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($twentyfourthPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'TwentyThirdTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_twentythird.py" $out
 if($LASTEXITCODE){throw 'TwentyThird native extraction failed'}
 $inputs+=@("$out/TwentyThirdNativeScripts.inc","$repo/tools/tests/prepare_fifteenth.py","$repo/src/server/game/Entities/Object/ObjectGuid.h","$repo/src/server/game/Entities/Pet/PetDefines.h","$repo/src/server/game/Movement/MotionMaster.h","$repo/tools/tests/prepare_twentythird.py","$repo/tools/tests/TwentyThirdTenRepairs.json")
 $twentythirdPlan=Get-Content "$repo/tools/tests/TwentyThirdTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($twentythirdPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'TwentySecondTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_twentysecond.py" $out
 if($LASTEXITCODE){throw 'TwentySecond native extraction failed'}
 $inputs+=@("$out/TwentySecondNativeScripts.inc","$repo/tools/tests/prepare_fifteenth.py","$repo/src/server/game/Entities/Object/ObjectGuid.h","$repo/src/server/game/Entities/Pet/PetDefines.h","$repo/src/server/game/Movement/MotionMaster.h","$repo/tools/tests/prepare_twentysecond.py","$repo/tools/tests/TwentySecondTenRepairs.json")
 $twentysecondPlan=Get-Content "$repo/tools/tests/TwentySecondTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($twentysecondPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'TwentyFirstTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_twentyfirst.py" $out
 if($LASTEXITCODE){throw 'TwentyFirst native extraction failed'}
 $inputs+=@("$out/TwentyFirstNativeScripts.inc","$repo/tools/tests/prepare_fifteenth.py","$repo/src/server/game/Entities/Object/ObjectGuid.h","$repo/tools/tests/prepare_twentyfirst.py","$repo/tools/tests/TwentyFirstTenRepairs.json")
 $twentyfirstPlan=Get-Content "$repo/tools/tests/TwentyFirstTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($twentyfirstPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'TwentiethTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_twentieth.py" $out
 if($LASTEXITCODE){throw 'Twentieth native extraction failed'}
 $inputs+=@("$out/TwentiethNativeScripts.inc","$repo/tools/tests/prepare_fifteenth.py","$repo/src/server/game/Entities/Object/ObjectGuid.h","$repo/tools/tests/prepare_twentieth.py","$repo/tools/tests/TwentiethTenRepairs.json")
 $twentiethPlan=Get-Content "$repo/tools/tests/TwentiethTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($twentiethPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'NineteenthTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_nineteenth.py" $out
 if($LASTEXITCODE){throw 'Nineteenth native extraction failed'}
 $inputs+=@("$out/NineteenthNativeScripts.inc","$repo/tools/tests/prepare_fifteenth.py","$repo/tools/tests/prepare_nineteenth.py","$repo/tools/tests/NineteenthTenRepairs.json")
 $nineteenthPlan=Get-Content "$repo/tools/tests/NineteenthTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($nineteenthPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'EighteenthTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_eighteenth.py" $out
 if($LASTEXITCODE){throw 'Eighteenth native extraction failed'}
 $inputs+=@("$out/EighteenthNativeScripts.inc","$repo/tools/tests/prepare_eighteenth.py","$repo/tools/tests/EighteenthTenRepairs.json")
 $eighteenthPlan=Get-Content "$repo/tools/tests/EighteenthTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($eighteenthPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'SeventeenthTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_seventeenth.py" $out
 if($LASTEXITCODE){throw 'Seventeenth native extraction failed'}
 $inputs+=@("$out/SeventeenthNativeScripts.inc","$repo/tools/tests/prepare_seventeenth.py","$repo/tools/tests/SeventeenthTenRepairs.json")
 $seventeenthPlan=Get-Content "$repo/tools/tests/SeventeenthTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($seventeenthPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'SixteenthTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_sixteenth.py" $out
 if($LASTEXITCODE){throw 'Sixteenth native extraction failed'}
 $inputs+=@("$out/SixteenthNativeScripts.inc","$out/SixteenthHatchling.inc","$repo/tools/tests/prepare_fifteenth.py","$repo/tools/tests/prepare_sixteenth.py","$repo/tools/tests/SixteenthTenRepairs.json")
 $sixteenthPlan=Get-Content "$repo/tools/tests/SixteenthTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($sixteenthPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'FifteenthTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_fifteenth.py" $out
 if($LASTEXITCODE){throw 'Fifteenth native extraction failed'}
 $inputs+=@("$out/FifteenthNativeScripts.inc","$repo/tools/tests/prepare_fifteenth.py","$repo/tools/tests/FifteenthTenRepairs.json")
 $fifteenthPlan=Get-Content "$repo/tools/tests/FifteenthTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($fifteenthPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'FourteenthTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_fourteenth.py" $out
 if($LASTEXITCODE){throw 'Fourteenth native extraction failed'}
 $inputs+=@("$out/FourteenthNativeScripts.inc","$repo/tools/tests/prepare_fourteenth.py","$repo/tools/tests/FourteenthTenRepairs.json")
 $fourteenthPlan=Get-Content "$repo/tools/tests/FourteenthTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($fourteenthPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'ThirteenthTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_thirteenth.py" $out
 if($LASTEXITCODE){throw 'Thirteenth native extraction failed'}
 $inputs+=@("$out/ThirteenthNativeScripts.inc","$repo/tools/tests/prepare_thirteenth.py","$repo/tools/tests/ThirteenthTenRepairs.json")
 $thirteenthPlan=Get-Content "$repo/tools/tests/ThirteenthTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($thirteenthPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'TwelfthTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_twelfth.py" $out
 if($LASTEXITCODE){throw 'Twelfth native extraction failed'}
 $inputs+=@("$out/TwelfthNativeScripts.inc","$repo/tools/tests/prepare_twelfth.py","$repo/tools/tests/TwelfthTenRepairs.json")
 $twelfthPlan=Get-Content "$repo/tools/tests/TwelfthTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($twelfthPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'EleventhTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_eleventh.py" $out
 if($LASTEXITCODE){throw 'Eleventh native extraction failed'}
 $inputs+=@("$out/EleventhSetups.inc","$out/EleventhConstants.inc","$repo/tools/tests/prepare_eleventh.py","$repo/tools/tests/EleventhTenRepairs.json")
 $eleventhPlan=Get-Content "$repo/tools/tests/EleventhTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($eleventhPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'TenthTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_tenth_native.py" $out
 if($LASTEXITCODE){throw 'Tenth native extraction failed'}
 $inputs+=@("$out/TenthNativeScripts.inc","$repo/tools/tests/prepare_tenth_native.py","$repo/tools/tests/TenthTenRepairs.json")
 $tenthPlan=Get-Content "$repo/tools/tests/TenthTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($tenthPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'NinthTenNative'){
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/tests/prepare_ninth_native.py" $out
 if($LASTEXITCODE){throw 'Ninth native extraction failed'}
 $inputs+=@("$out/NinthNativeScripts.inc","$repo/tools/tests/prepare_ninth_native.py","$repo/tools/tests/NinthTenRepairs.json")
 $ninthPlan=Get-Content "$repo/tools/tests/NinthTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($ninthPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'EighthTenNative'){
 & "$PSScriptRoot/Prepare-EighthTenNative.ps1" -OutputDirectory $out
 $inputs+=@("$out/EighthResolvers.inc","$repo/tools/tests/Prepare-EighthTenNative.ps1","$repo/tools/tests/EighthTenRepairs.json")
 $eighthPlan=Get-Content "$repo/tools/tests/EighthTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($eighthPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'SeventhTenNative'){
 & "$PSScriptRoot/Prepare-SeventhTenNative.ps1" -OutputDirectory $out
 $inputs+=@("$out/SeventhResolvers.inc","$repo/tools/tests/Prepare-SeventhTenNative.ps1","$repo/tools/tests/SeventhTenRepairs.json")
 $seventhPlan=Get-Content "$repo/tools/tests/SeventhTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($seventhPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'SixthTenNative'){
 & "$PSScriptRoot/Prepare-SixthTenNative.ps1" -OutputDirectory $out
 $inputs+=@("$out/SixthDumpHelpers.inc","$out/SixthScripts.inc","$out/SixthChain.inc","$out/SixthRestore.inc","$repo/tools/tests/Prepare-SixthTenNative.ps1","$repo/tools/tests/SixthTenRepairs.json")
 $sixthPlan=Get-Content "$repo/tools/tests/SixthTenRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($sixthPlan.paths|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'FifthTenNative'){
 & "$PSScriptRoot/Prepare-FifthTenNative.ps1" -OutputDirectory $out
 $inputs+=@("$out/FifthTenScripts.inc","$repo/tools/tests/Prepare-FifthTenNative.ps1","$repo/tools/tests/FifthTenRepairs.json","$repo/src/server/scripts/Spells/spell_shaman.cpp","$repo/src/server/scripts/Spells/spell_dh.cpp","$repo/src/server/scripts/Spells/spell_paladin.cpp","$repo/src/server/game/Movement/MotionMaster.h","$repo/src/server/game/Movement/MotionMaster.cpp")
}
if($AdditionalProbe -eq 'FourthTenSpellNative'){
 & "$PSScriptRoot/Prepare-FourthTenSpellNative.ps1" -OutputDirectory $out
 $inputs+=@("$out/FourthTenScripts.inc","$out/FourthTenBindings.inc","$out/FourthVantusTables.inc","$out/FourthValorPredicate.inc","$repo/tools/tests/Prepare-FourthTenSpellNative.ps1","$repo/tools/tests/FourthTenSpellRepairs.json","$repo/tools/tests/FourthTenVantus.json","$repo/src/server/scripts/Legion/VantusRuneLifecycle.h")
 $plan=Get-Content "$repo/tools/tests/FourthTenSpellRepairs.json" -Raw|ConvertFrom-Json
 $inputs+=@($plan.edits.path|Sort-Object -Unique|ForEach-Object {"$repo/$_"})
 $inputs+=@((Get-Content "$repo/tools/tests/FourthTenVantus.json" -Raw|ConvertFrom-Json).header|ForEach-Object {"$repo/$_"})
}
if($AdditionalProbe -eq 'WarswordsStageFourNative'){
 $path='src/server/scripts/Scenario/Artifacts/Warrior/Warswords/instance_warswords.cpp'
 $instance=if($AdditionalBaseline){(& git -C $repo show "6a5b1f80b:$path") -join "`n"}else{Get-Content "$repo/$path" -Raw};if(!$instance){throw 'Missing stage4 instance'}
 [IO.File]::WriteAllText("$out/WarswordsStageFourInstance.inc",$instance)
 $inputs+=@("$out/WarswordsStageFourInstance.inc","$repo/$path","$repo/src/server/scripts/Scenario/Artifacts/Warrior/Warswords/warswords_arrival.h","$repo/src/server/scripts/Scenario/Artifacts/Warrior/Warswords/warswords_waves.h")
}
if($AdditionalBaseline -and $AdditionalProbe -ne 'WarswordsStageFourNative'){if($AdditionalProbe -ne 'VehicleDeferredAdmissionNative'){throw 'Baseline requires deferred admission probe'};$pinned=(& git -C $repo show '6dc3bc59f:src/server/game/Entities/Vehicle/Vehicle.cpp') -join "`n";if($LASTEXITCODE){throw 'Missing pinned vehicle baseline'};[IO.File]::WriteAllText("$out/VehicleDeferredPinned.inc",$pinned);$inputs+="$out/VehicleDeferredPinned.inc"}
$hashes=@{};foreach($path in $inputs){$hashes[$path]=(Get-FileHash $path).Hash}
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$previousCompiler /DNDEBUG /Zi /Fd`"$out/compiler.pdb`" /I`"$out`" /I`"$repo/src/server/scripts/Legion`""


 if($AdditionalBaseline){$env:CL+=$(if($AdditionalProbe -eq 'WarswordsStageFourNative'){' /DWARSWORDS_STAGE_FOUR_BASELINE'}else{' /DVEHICLE_DEFERRED_BASELINE'})}
 if($AdditionalProbe -eq 'WarswordsStageFourNative'){$env:CL+=' /I"'+$repo+'/src/server/scripts/Scenario/Artifacts/Warrior/Warswords"'}
 if($SharasdalLadyCarryLive){$env:CL+=' /DSHARASDAL_CARRY_LIVE'}
 if($VehicleAuraCapacityBaseline){$env:CL+=' /DVEHICLE_AURA_CAPACITY_BASELINE'}
 if($SharasdalWaveBaseline){$env:CL+=' /DSHARASDAL_WAVE_BASELINE'}
 if($DevourerOutroBaseline){$env:CL+=' /DDEVOURER_OUTRO_BASELINE'}
 if($DevourerGraveyardBaseline){$env:CL+=' /DDEVOURER_GRAVEYARD_BASELINE'}
 if($KingslayersGraveyardBaseline){$env:CL+=' /DKINGSLAYERS_GRAVEYARD_BASELINE'}
 if($DreadbladesGraveyardBaseline){$env:CL+=' /DDREADBLADES_GRAVEYARD_BASELINE'}
 if($SharasdalEntranceBaseline){$env:CL+=' /DSHARASDAL_LEGACY_LOGIN_BASELINE'};if($SharasdalEntranceLive){$env:CL+=' /DSHARASDAL_ENTRANCE_LIVE'}
 if($SharasdalGraveyard){$env:CL+=' /I"'+$repo+'/src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal"'}
 if($SharasdalGraveyardBaseline){$env:CL+=' /DSHARASDAL_GRAVEYARD_BASELINE'}
 if($HolyGraveyardBaseline){$env:CL+=' /DHOLY_GRAVEYARD_BASELINE'}
 if($ShadowGraveyardBaseline){$env:CL+=' /DSHADOW_GRAVEYARD_BASELINE'}
 if($ArmsGraveyardBaseline){$env:CL+=' /DARMS_GRAVEYARD_BASELINE'}
 if($HodirGraveyardBaseline){$env:CL+=' /DHODIR_GRAVEYARD_BASELINE'}
 if($HodirLandingBaseline){$env:CL+=' /DHODIR_LANDING_BASELINE'};if($HodirLandingLive){$env:CL+=' /DHODIR_LANDING_LIVE'}
 if($MarksmanRecoveryBaseline){$env:CL+=" /DMARKSMAN_LOGIN_BASELINE"}
 if($BloodGraveyardBaseline){$env:CL+=' /DBLOOD_GRAVEYARD_BASELINE'}
 if($MarksmanGraveyardBaseline){$env:CL+=' /DMARKSMAN_GRAVEYARD_BASELINE'}
 if($TuureGraveyardBaseline){$env:CL+=' /DTUURE_GRAVEYARD_BASELINE'}
 if($TuureLegacyBaseline){$env:CL+=' /DTUURE_LEGACY_LOGIN_BASELINE'}
 if($TuureEntranceBaseline){$env:CL+=' /DTUURE_ENTRANCE_BASELINE'};if($TuureEntranceLive){$env:CL+=' /DTUURE_ENTRANCE_LIVE'}
 if($BloodEntranceBaseline){$env:CL+=' /DBLOOD_ENTRANCE_BASELINE'};if($BloodEntranceLive){$env:CL+=' /DBLOOD_ENTRANCE_LIVE'}
 if($MarksmanshipEntranceBaseline){$env:CL+=" /DMARKSMANSHIP_ENTRANCE_BASELINE"};if($MarksmanshipEntranceLive){$env:CL+=" /DMARKSMANSHIP_ENTRANCE_LIVE"}
 if($HunterRecoveryBaseline){$env:CL+=" /DSURVIVAL_LOGIN_BASELINE"}
 if($SurvivalEntranceBaseline){$env:CL+=" /DSURVIVAL_ENTRANCE_BASELINE"};if($SurvivalEntranceLive){$env:CL+=" /DSURVIVAL_ENTRANCE_LIVE"}
 if($SurvivalGraveyardBaseline){$env:CL+=" /DSURVIVAL_GRAVEYARD_BASELINE"}
 if($FuryGraveyard){$env:CL+=" /I`"$repo/src/server/scripts/Scenario/Artifacts/Warrior/Warswords`""};if($FuryGraveyardBaseline){$env:CL+=" /DFURY_GRAVEYARD_BASELINE"}
 if($ReofferBaseline){$env:CL+=" /DHRUTHNIR_REOFFER_BASELINE"}
 if($ResetBaseline){$env:CL+=" /DMAGNAR_RESET_BASELINE"}
 if($ChamberBaseline){$env:CL+=" /DHRUTHNIR_CHAMBER_BASELINE"}
 if($ReplacementBaseline){$env:CL+=" /DMAGNAR_REPLACEMENT_BASELINE"}
 if($WarriorRecoveryBaseline){$env:CL+=" /DWARRIOR_LOGIN_RECOVERY_BASELINE"}
 if($WarriorLoginBaseline){$env:CL+=" /DWARRIOR_LOGIN_BASELINE"}
 if($LoginBaseline){$env:CL+=" /DMAGNAR_LOGIN_BASELINE"}
 if($LoginLiveData){$env:CL+=" /DMAGNAR_LOGIN_LIVE"}
 if($DuelDeferredBaseline){$env:CL+=" /DMAGNAR_DUEL_DEFERRED_BASELINE"}
 if($DuelRecreationBaseline){$env:CL+=" /DMAGNAR_DUEL_RECREATION_BASELINE"}
 if($RecreationBaseline){$env:CL+=" /DMAGNAR_RECREATION_BASELINE"}
 if($CombatBaseline){$env:CL+=" /DMAGNAR_COMBAT_BASELINE"}
 if($PrimaryBaseline){$env:CL+=" /DHRUTHNIR_PRIMARY_BASELINE"}
 if($CursorBaseline){$env:CL+=" /DHRUTHNIR_CURSOR_BASELINE"}
 if($ColdRouteBaseline){$env:CL+=" /DHRUTHNIR_COLD_BASELINE"}
 if($BarrierBaseline){$env:CL+=" /DHRUTHNIR_BARRIER_BASELINE"}
 if($ApproachBaseline){$env:CL+=" /DHRUTHNIR_APPROACH_BASELINE"}
 if($PillikBaseline){$env:CL+=" /DHRUTHNIR_PILLIK_BASELINE"}
 if($EntryBaseline){$env:CL+=" /DHRUTHNIR_ENTRY_BASELINE"}
 if($Baseline){$env:CL+=' /DMAGNAR_READINESS_BASELINE'};$env:_LINK_="$previousLink /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{& "$repo/tools/tests/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile "../tests/$probe.cpp" -ProbeName $probe -OutputFile "$out/native.tsv" *> "$out/catalog.log"
  if(!$? -or $LASTEXITCODE){throw "Native Protection readiness regression failed: $out/catalog.log"}
 }}finally{Pop-Location}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$previousCompiler;$env:_LINK_=$previousLink}
if((Get-FileHash $configPath).Hash -ne $hash){throw 'Canonical configuration changed'}

foreach($path in $inputs){if((Get-FileHash $path).Hash -ne $hashes[$path]){throw 'Native Protection readiness source/final-link inputs changed during execution'}}
@{Inputs=$hashes;Probe=(Get-FileHash "$repo/build-extractors/runtime-test/legion-legendary-catalog/$probe.exe").Hash;NativeReleaseNotAsan=$true}|ConvertTo-Json|Set-Content "$out/native-inputs.json"
Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|CASE|CONTROL|COUNTERFACTUAL)'}
