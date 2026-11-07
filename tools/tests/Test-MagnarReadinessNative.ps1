# Real native Protection wave readiness and Kill/owner progression; no persistent character writes.
param([switch]$Baseline,[switch]$StartRecovery,[switch]$Pillik,[switch]$PillikBaseline,[switch]$EntryBaseline,[switch]$Approach,[switch]$ApproachBaseline,[switch]$Reoffer,[switch]$ReofferBaseline,[switch]$ResetRecovery,[switch]$ResetBaseline,[switch]$ChamberRecovery,[switch]$ChamberBaseline)
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
if($EntryBaseline){$Pillik=$true}
if($EntryBaseline -and $PillikBaseline){throw 'Choose one pinned Pillik counterfactual'}
if($PillikBaseline){$Pillik=$true}
if($StartRecovery -and $Pillik){throw "Choose start recovery or the earlier Pillik handoff"}
if($Baseline -and ($StartRecovery -or $Pillik)){throw "Pinned counterfactual is separate from current start recovery"}
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$family=if($ChamberRecovery){"hruthnir-chamber-native"}elseif($ResetRecovery){"magnar-reset-native"}elseif($Reoffer){"hruthnir-reoffer-native"}elseif($Approach){"hruthnir-approach-native"}elseif($Pillik){"hruthnir-pillik-native"}elseif($StartRecovery -or $Pillik){"hruthnir-start-native"}else{"magnar-readiness-native"}
$probe=if($ChamberRecovery){"HruthnirChamberNative"}elseif($ResetRecovery){"MagnarResetNative"}elseif($Reoffer){"HruthnirReofferNative"}elseif($Approach){"HruthnirApproachNative"}elseif($Pillik){"HruthnirPillikNative"}elseif($StartRecovery -or $Pillik){"HruthnirStartNative"}else{"MagnarReadinessNative"}
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
[IO.File]::WriteAllText("$out/NativeAltheaQuest.inc",$query+' " WHERE ID IN(40710,42504,42627,40043,39191)"')
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
if($ChamberBaseline){$source=(& git -C $repo show "827bb28aa:$path") -join "`n";if(!$source){throw 'Pinned fresh chamber source missing'}}
[IO.File]::WriteAllText("$out/MagnarNative.inc",(Block $source 'Position const trashPos[12]')+';'+(Block $source 'class npc_magnar_icebreaker : public CreatureScript')+';')
if($StartRecovery -or $Pillik){
 $resetBridge=if($source.Contains('void MagnarStart::ResetOffer(')){Block $source 'void MagnarStart::ResetOffer('}else{''}
 [IO.File]::WriteAllText("$out/HruthnirNative.inc",(Block $source 'namespace HruthnirGossip')+(Block $source 'class npc_hrutnir : public CreatureScript')+';'+(Block $source 'void MagnarStart::Rejoin(')+$resetBridge)
 $shared=if($PillikBaseline){(& git -C $repo show "cb1300a9f:src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/instance_shields_rest.cpp") -join "`n"}else{Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/instance_shields_rest.cpp" -Raw}
 if($ChamberBaseline){$shared=(& git -C $repo show '827bb28aa:src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/instance_shields_rest.cpp') -join "`n"}
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
if($Approach){$inputs+=@("$repo/tools/tests/HruthnirApproachNative.cpp","$repo/src/server/game/Movement/MovementGenerators/WaypointMovementGenerator.h")}
if($Pillik){$inputs+=@("$repo/tools/tests/HruthnirPillikNative.cpp")}
if($StartRecovery -or $Pillik){$inputs+=@("$repo/src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/hruthnir_pillik.h","$out/HruthnirNative.inc","$out/ShieldsNative.inc")}
$inputs+=@("$out/MagnarNative.inc")
$hashes=@{};foreach($path in $inputs){$hashes[$path]=(Get-FileHash $path).Hash}
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$previousCompiler /DNDEBUG /Zi /Fd`"$out/compiler.pdb`" /I`"$out`" /I`"$repo/src/server/scripts/Legion`""


 if($ReofferBaseline){$env:CL+=" /DHRUTHNIR_REOFFER_BASELINE"}
 if($ResetBaseline){$env:CL+=" /DMAGNAR_RESET_BASELINE"}
 if($ChamberBaseline){$env:CL+=" /DHRUTHNIR_CHAMBER_BASELINE"}
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
