param([switch]$Baseline,[switch]$VerifyLiveData,[string]$MigrationPath="")
if($Baseline -and $VerifyLiveData){throw "Pinned counterfactual cannot certify live data"}
$StartRecovery=$true;$Pillik=$false;$Approach=$false;$ApproachBaseline=$false;$PillikBaseline=$false;$EntryBaseline=$false;$ReadinessBaseline=$false
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$MigrationPath){$MigrationPath="$repo/sql/updates/world/2026_11_06_06_hruthnir_initial_movement.sql"}
$MigrationPath=[IO.Path]::GetFullPath($MigrationPath)
$family="hruthnir-spawn-native"
$probe="HruthnirSpawnNative"
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
$source=if($ApproachBaseline){(& git -C $repo show "58134ab60:$path") -join "`n"}elseif($EntryBaseline){(& git -C $repo show "7dc957758:$path") -join "`n"}elseif($PillikBaseline){(& git -C $repo show "cb1300a9f:$path") -join "`n"}elseif($ReadinessBaseline){(& git -C $repo show "fb01c9a6a:$path") -join "`n"}else{Get-Content "$repo/$path" -Raw};if(!$source){throw 'Actual Magnar source missing'}
[IO.File]::WriteAllText("$out/MagnarNative.inc",(Block $source 'Position const trashPos[12]')+';'+(Block $source 'class npc_magnar_icebreaker : public CreatureScript')+';')
if($StartRecovery -or $Pillik){
 [IO.File]::WriteAllText("$out/HruthnirNative.inc",(Block $source 'namespace HruthnirGossip')+(Block $source 'class npc_hrutnir : public CreatureScript')+';'+(Block $source 'void MagnarStart::Rejoin('))
 $shared=if($PillikBaseline){(& git -C $repo show "cb1300a9f:src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/instance_shields_rest.cpp") -join "`n"}else{Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/instance_shields_rest.cpp" -Raw}
 [IO.File]::WriteAllText("$out/ShieldsNative.inc",(Block $shared 'class instance_shields_rest : public InstanceMapScript')+';')
}
$previousMigration=$env:LEGION_SPAWN_SQL;$previousVerify=$env:LEGION_SPAWN_VERIFY;
$previous=$env:LEGION_DIALOGUE_CONFIG;$previousCompiler=$env:CL;$previousLink=$env:_LINK_
$inputs=@("$repo/tools/tests/SlaghammerCompanionNative.cpp","$repo/src/server/game/Conditions/ConditionMgr.cpp","$repo/src/server/game/Conditions/ConditionMgr.h","$repo/src/server/game/Entities/Object/Object.h","$repo/src/server/game/Entities/Object/Object.cpp","$repo/src/server/game/Maps/Map.cpp","$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp","$repo/tools/tests/AltheaIntroNative.cpp","$repo/tools/tests/MagnarReadinessNative.cpp","$repo/src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/EarthWarder.cpp","$repo/src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/instance_silver_hand.cpp","$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/instance_kingslayers.cpp","$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/KingslayersGuardProgress.h","$repo/src/server/game/Entities/Player/Player.cpp","$repo/src/server/game/Spells/SpellEffects.cpp","$repo/build-extractors/src/server/game/Release/game.lib","$repo/build-extractors/src/server/scripts/Release/scripts.lib")
# Reusable MSBuild workers can remain idle after completion. Certify the actual
# source and final-link library bytes before/after the sequential native run.
$inputs+=@("$repo/src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/magnar_start.h")
if($StartRecovery -or $Pillik){$inputs+=@("$repo/tools/tests/HruthnirStartNative.cpp","$repo/src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/instance_shields_rest.cpp","$repo/src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/shields_rest.h","$repo/src/server/game/Entities/Object/QuestObjectScope.h")}
$inputs+=@($MigrationPath,"$repo/tools/tests/HruthnirSpawnNative.cpp","$repo/src/server/game/Globals/ObjectMgr.cpp","$repo/src/server/game/Entities/Creature/Creature.cpp")
if($Approach){$inputs+=@("$repo/tools/tests/HruthnirApproachNative.cpp","$repo/src/server/game/Movement/MovementGenerators/WaypointMovementGenerator.h")}
if($Pillik){$inputs+=@("$repo/tools/tests/HruthnirPillikNative.cpp")}
if($StartRecovery -or $Pillik){$inputs+=@("$repo/src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/hruthnir_pillik.h","$out/HruthnirNative.inc","$out/ShieldsNative.inc")}
$inputs+=@("$out/MagnarNative.inc")
$hashes=@{};foreach($path in $inputs){$hashes[$path]=(Get-FileHash $path).Hash}
try{
 $env:LEGION_SPAWN_SQL=$MigrationPath;$env:LEGION_SPAWN_VERIFY=if($VerifyLiveData){"1"}else{"0"}
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$previousCompiler /DNDEBUG /Zi /Fd`"$out/compiler.pdb`" /I`"$out`" /I`"$repo/src/server/scripts/Legion`""


 if($Baseline){$env:CL+=" /DHRUTHNIR_SPAWN_BASELINE"}
 if($ApproachBaseline){$env:CL+=" /DHRUTHNIR_APPROACH_BASELINE"}
 if($PillikBaseline){$env:CL+=" /DHRUTHNIR_PILLIK_BASELINE"}
 if($EntryBaseline){$env:CL+=" /DHRUTHNIR_ENTRY_BASELINE"}
 if($ReadinessBaseline){$env:CL+=' /DMAGNAR_READINESS_BASELINE'};$env:_LINK_="$previousLink /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{& "$repo/tools/tests/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile "../tests/$probe.cpp" -ProbeName $probe -OutputFile "$out/native.tsv" *> "$out/catalog.log"
  if(!$? -or $LASTEXITCODE){throw "Native Protection readiness regression failed: $out/catalog.log"}
 }}catch{
  if(!$Baseline){throw}
  $d=Get-Content "$out/evidence/diagnostics-after.json" -Raw|ConvertFrom-Json
  $warnings=@($d.AppendedDBErrors|ForEach-Object {$_.Text -split "`n"}|Where-Object {$_.Trim()})
  $unexpected=@(@($warnings)+@($d.ServerDiagnostics)|Where-Object {$_ -notmatch ' WARN .*WaypointMovementGenerator::LoadPath:.*Entry: 96468.*waypoint path id: 0'})
  if(!$d.Completed -or @($d.ChangedCrashes).Count -or @($d.ServerDiagnostics).Count -ne 2 -or @($d.AppendedDBErrors).Count -ne 1 -or $warnings.Count -ne 2 -or $unexpected.Count){throw}
  'COUNTERFACTUAL diagnostics: exactly two original missing-path0 warnings retained; no unexpected crash/DBErrors/server diagnostic; this is not an ordinary successful run'
 }finally{Pop-Location}
}finally{$env:LEGION_SPAWN_SQL=$previousMigration;$env:LEGION_SPAWN_VERIFY=$previousVerify;$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$previousCompiler;$env:_LINK_=$previousLink}
if((Get-FileHash $configPath).Hash -ne $hash){throw 'Canonical configuration changed'}

foreach($path in $inputs){if((Get-FileHash $path).Hash -ne $hashes[$path]){throw 'Native Protection readiness source/final-link inputs changed during execution'}}
@{Inputs=$hashes;Probe=(Get-FileHash "$repo/build-extractors/runtime-test/legion-legendary-catalog/$probe.exe").Hash;NativeReleaseNotAsan=$true}|ConvertTo-Json|Set-Content "$out/native-inputs.json"
Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|CASE|CONTROL|COUNTERFACTUAL)'}
