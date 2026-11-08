# Real native Protection wave readiness and Kill/owner progression; no persistent character writes.
param([switch]$HodirGraveyardBaseline,[switch]$HodirGraveyard,[switch]$HodirLandingBaseline,[switch]$HodirLandingLive,[switch]$HodirLanding,[switch]$BloodGraveyardBaseline,[switch]$BloodGraveyard,[switch]$MarksmanGraveyardBaseline,[switch]$MarksmanGraveyard,[switch]$TuureGraveyardBaseline,[switch]$TuureGraveyard,[switch]$TuureLegacyBaseline,[switch]$TuureLegacy,[switch]$TuureEntranceBaseline,[switch]$TuureEntranceLive,[switch]$TuureEntrance,[switch]$BloodEntranceBaseline,[switch]$BloodEntranceLive,[switch]$BloodEntrance,[switch]$MarksmanEarned,[switch]$MarksmanRecoveryBaseline,[switch]$MarksmanRecovery,[switch]$MarksmanshipEntranceBaseline,[switch]$MarksmanshipEntranceLive,[switch]$MarksmanshipEntrance,[switch]$HunterRecoveryBaseline,[switch]$HunterRecovery,[switch]$SurvivalEntranceBaseline,[switch]$SurvivalEntranceLive,[switch]$SurvivalEntrance,[switch]$SurvivalGraveyardBaseline,[switch]$SurvivalGraveyard,[switch]$FuryGraveyardBaseline,[switch]$FuryGraveyard,[switch]$WarriorRecoveryBaseline,[switch]$WarriorRecovery,[switch]$WarriorLoginBaseline,[switch]$WarriorLogin,[switch]$LoginBaseline,[switch]$LoginLiveData,[switch]$LoginRecovery,[switch]$DuelDeferredBaseline,[switch]$DuelRecreation,[switch]$DuelRecreationBaseline,[switch]$Recreation,[switch]$RecreationBaseline,[switch]$CombatReplacement,[switch]$CombatBaseline,[switch]$PrimaryApproach,[switch]$PrimaryBaseline,[switch]$Cursor,[switch]$CursorBaseline,[switch]$Replacement,[switch]$ReplacementBaseline,[switch]$ColdRoute,[switch]$ColdRouteBaseline,[switch]$Baseline,[switch]$StartRecovery,[switch]$Pillik,[switch]$PillikBaseline,[switch]$EntryBaseline,[switch]$Approach,[switch]$ApproachBaseline,[switch]$Reoffer,[switch]$ReofferBaseline,[switch]$ResetRecovery,[switch]$ResetBaseline,[switch]$ChamberRecovery,[switch]$ChamberBaseline,[switch]$Barrier,[switch]$BarrierBaseline)
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
$family=if($HodirGraveyard){"hodir-graveyard-native"}elseif($HodirLanding){"hodir-landing-native"}elseif($BloodGraveyard){"blood-graveyard-native"}elseif($MarksmanGraveyard){"marksmanship-graveyard-native"}elseif($TuureGraveyard){"tuure-graveyard-native"}elseif($TuureLegacy){"tuure-legacy-login-native"}elseif($TuureEntrance){"tuure-entrance-native"}elseif($BloodEntrance){"blood-entrance-native"}elseif($MarksmanEarned){"marksmanship-earned-native"}elseif($MarksmanRecovery){"marksmanship-login-recovery-native"}elseif($MarksmanshipEntrance){"marksmanship-entrance-native"}elseif($HunterRecovery){"survival-login-recovery-native"}elseif($SurvivalEntrance){"survival-entrance-native"}elseif($SurvivalGraveyard){"survival-graveyard-native"}elseif($FuryGraveyard){"warswords-graveyard-native"}elseif($WarriorRecovery){"warrior-login-recovery-native"}elseif($WarriorLogin){"warrior-scenario-resume-native"}elseif($LoginRecovery){"magnar-login-native"}elseif($DuelRecreation){"magnar-duel-recreation-native"}elseif($Recreation){"magnar-recreation-native"}elseif($CombatReplacement){"magnar-combat-native"}elseif($PrimaryApproach){"hruthnir-primary-native"}elseif($Cursor){"hruthnir-cursor-native"}elseif($Replacement){"magnar-replacement-native"}elseif($ColdRoute){"hruthnir-cold-route-native"}elseif($Barrier){"hruthnir-barrier-native"}elseif($ChamberRecovery){"hruthnir-chamber-native"}elseif($ResetRecovery){"magnar-reset-native"}elseif($Reoffer){"hruthnir-reoffer-native"}elseif($Approach){"hruthnir-approach-native"}elseif($Pillik){"hruthnir-pillik-native"}elseif($StartRecovery -or $Pillik){"hruthnir-start-native"}else{"magnar-readiness-native"}
$probe=if($HodirGraveyard){"HodirGraveyardNative"}elseif($HodirLanding){"HodirLandingNative"}elseif($BloodGraveyard){"BloodGraveyardNative"}elseif($MarksmanGraveyard){"MarksmanshipGraveyardNative"}elseif($TuureGraveyard){"TuureGraveyardNative"}elseif($TuureLegacy){"TuureLegacyLoginNative"}elseif($TuureEntrance){"TuureEntranceNative"}elseif($BloodEntrance){"BloodEntranceNative"}elseif($MarksmanEarned){"MarksmanshipEarnedReturnNative"}elseif($MarksmanRecovery){"MarksmanshipLoginRecoveryNative"}elseif($MarksmanshipEntrance){"MarksmanshipEntranceNative"}elseif($HunterRecovery){"SurvivalLoginRecoveryNative"}elseif($SurvivalEntrance){"SurvivalEntranceNative"}elseif($SurvivalGraveyard){"SurvivalGraveyardNative"}elseif($FuryGraveyard){"WarswordsGraveyardNative"}elseif($WarriorRecovery){"WarriorLoginRecoveryNative"}elseif($WarriorLogin){"WarriorScenarioResumeNative"}elseif($LoginRecovery){"MagnarLoginNative"}elseif($DuelRecreation){"MagnarDuelRecreationNative"}elseif($Recreation){"MagnarRecreationNative"}elseif($CombatReplacement){"MagnarCombatReplacementNative"}elseif($PrimaryApproach){"HruthnirPrimaryApproachNative"}elseif($Cursor){"HruthnirCursorNative"}elseif($Replacement){"MagnarReplacementNative"}elseif($ColdRoute){"HruthnirColdRouteNative"}elseif($Barrier){"HruthnirBarrierNative"}elseif($ChamberRecovery){"HruthnirChamberNative"}elseif($ResetRecovery){"MagnarResetNative"}elseif($Reoffer){"HruthnirReofferNative"}elseif($Approach){"HruthnirApproachNative"}elseif($Pillik){"HruthnirPillikNative"}elseif($StartRecovery -or $Pillik){"HruthnirStartNative"}else{"MagnarReadinessNative"}
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
if($HodirLanding -or $HodirGraveyard){$questIds+=',42974'}
if($MarksmanshipEntrance -or $MarksmanRecovery -or $MarksmanGraveyard){$questIds+=',40419'}
if($BloodEntrance -or $BloodGraveyard){$questIds+=',40740'}
if($TuureEntrance -or $TuureGraveyard){$questIds+=',41993'}
[IO.File]::WriteAllText("$out/NativeAltheaQuest.inc",$query+' " WHERE ID IN('+$questIds+')"')
$configPath="$repo/build-extractors/bin/Release/worldserver.conf";$hash=(Get-FileHash $configPath).Hash;$config=Get-Content $configPath -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'));[IO.File]::WriteAllText("$out/worldserver.conf",$config)
[IO.File]::WriteAllText("$out/AltheaIntro.inc",(Block $source 'class npc_sister_althea_ebonlocke : public CreatureScript')+';')
$instanceSource=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/instance_kingslayers.cpp" -Raw
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
$hashes=@{};foreach($path in $inputs){$hashes[$path]=(Get-FileHash $path).Hash}
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$previousCompiler /DNDEBUG /Zi /Fd`"$out/compiler.pdb`" /I`"$out`" /I`"$repo/src/server/scripts/Legion`""


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
