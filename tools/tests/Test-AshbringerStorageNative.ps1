# Actual180850/199827 native storage,quest-scoped presentation and credit.
param([switch]$Baseline,[switch]$VerifyLiveData)
if($Baseline -and $VerifyLiveData){throw 'Counterfactual cannot certify live bindings'}
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/ashbringer-storage-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force "$out/logs"|Out-Null
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing actual $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
$source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp" -Raw
$ai=Block $source 'struct npc_sister_althea_ebonlockeAI : ScriptedAI'
foreach($pair in @(@('AltheaVictory','Player* GetVictoryPlayer('),@('Althea','void JustDied(Unit* who) override'))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $ai $pair[1]))}
[IO.File]::WriteAllText("$out/AltheaActor.inc",(Block $source 'class AltheaDeathConversation final : public Conversation')+';')
$old=(& git -C $repo show 'eb96dd4e6:src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned nearest-player baseline missing'}
# Shared bootstrap declares these pinned victory prerequisite helpers. They
# must compile, but this storage probe never invokes the old door/death hooks.
[IO.File]::WriteAllText("$out/AltheaDoor.inc",(Block (Block $old 'struct go_kingslayers_doorAI : public GameObjectAI') 'bool GossipHello('))
[IO.File]::WriteAllText("$out/AltheaBeforeCredit.inc",(Block (Block $old 'struct npc_sister_althea_ebonlockeAI : ScriptedAI') 'void JustDied(Unit* who) override'))
$quest=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
$query=[regex]::Match($quest,'(?s)void QuestDataStoreMgr::LoadQuests\(\).*?WorldDatabase.Query\(("SELECT ".*?" FROM quest_template")\);').Groups[1].Value
if(!$query){throw 'Actual native quest query missing'}
[IO.File]::WriteAllText("$out/NativeAltheaQuest.inc",$query+' " WHERE ID IN(40710,42504,42627,38376)"')
$configPath="$repo/build-extractors/bin/Release/worldserver.conf";$hash=(Get-FileHash $configPath).Hash;$config=Get-Content $configPath -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'));[IO.File]::WriteAllText("$out/worldserver.conf",$config)
[IO.File]::WriteAllText("$out/AltheaIntro.inc",(Block $source 'class npc_sister_althea_ebonlocke : public CreatureScript')+';')
$instanceSource=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/instance_kingslayers.cpp" -Raw
[IO.File]::WriteAllText("$out/KingslayersInstance.inc",(Block $instanceSource 'class instance_kingslayers : public InstanceMapScript')+';')
$instancePath='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/instance_silver_hand.cpp'
$instance=Get-Content "$repo/$instancePath" -Raw
if(!$instance){throw 'Actual/pinned shared instance source missing'}
[IO.File]::WriteAllText("$out/SilverHandInstance.inc",(Block $instance 'class instance_silver_hand : public InstanceMapScript')+';')
$paladinPath='src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/broken_shore.cpp'
$paladin=[IO.File]::ReadAllText("$repo/$paladinPath")
if($Baseline){$paladin=(& git -C $repo show "027217ea1:$paladinPath") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-storage source missing'}}
$header=[IO.File]::ReadAllText("$repo/src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/broken_shore.h")
$actor=(Block $header 'enum Data')+';'+(Block $paladin 'enum eSpells')+';'+(Block $paladin 'namespace AshbringerAcquisition')+[Environment]::NewLine+$(if(!$Baseline){'#include "ashbringer_storage.h"'+"
"}else{''})+(Block $paladin 'class go_bs_the_ashbringer :')+';'
[IO.File]::WriteAllText("$out/AshbringerStorageActor.inc",$actor)
[IO.File]::WriteAllText("$out/AshbringerStorageConstants.inc",'')
$previous=$env:LEGION_DIALOGUE_CONFIG;$previousCompiler=$env:CL;$previousLink=$env:_LINK_
$inputs=@("$repo/tools/tests/SlaghammerCompanionNative.cpp","$repo/src/server/game/Conditions/ConditionMgr.cpp","$repo/src/server/game/Conditions/ConditionMgr.h","$repo/src/server/game/Entities/Object/Object.h","$repo/src/server/game/Entities/Object/Object.cpp","$repo/src/server/game/Maps/Map.cpp","$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp","$repo/tools/tests/AltheaIntroNative.cpp","$repo/tools/tests/AshbringerStorageNative.cpp","$repo/src/server/scripts/Legion/ClassHalls/warrior_pickup_return.h","$repo/src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/instance_silver_hand.cpp","$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/instance_kingslayers.cpp","$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/KingslayersGuardProgress.h","$repo/src/server/game/Entities/Player/Player.cpp","$repo/src/server/game/Spells/SpellEffects.cpp","$repo/build-extractors/src/server/game/Release/game.lib","$repo/build-extractors/src/server/scripts/Release/scripts.lib")
# Reusable MSBuild workers can remain idle after completion. Certify the actual
# source and final-link library bytes before/after the sequential native run.
$inputs+=@("$repo/src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/ashbringer_storage.h","$repo/$paladinPath","$out/AshbringerStorageActor.inc","$out/AshbringerStorageConstants.inc","$repo/src/server/game/Entities/Unit/Unit.h","$repo/src/server/game/Entities/Player/Player.cpp")
$hashes=@{};foreach($path in $inputs){$hashes[$path]=(Get-FileHash $path).Hash}
Copy-Item -LiteralPath "$PSScriptRoot/AshbringerStorageNative.cpp" -Destination "$out/ProbeSource.cpp"
Copy-Item -LiteralPath "$repo/$paladinPath" -Destination "$out/ProductionSource.cpp"
@{Inputs=$hashes;PinnedCallbackBaseline=[bool]$Baseline;ProbeSourceHash=(Get-FileHash "$out/ProbeSource.cpp").Hash;ProductionSourceHash=(Get-FileHash "$out/ProductionSource.cpp").Hash}|ConvertTo-Json|Set-Content "$out/native-before-inputs.json"
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$previousCompiler /DNDEBUG /Zi /I`"$out`" /I`"$repo/src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore`" /I`"$repo/src/server/scripts/Legion`""


 if($Baseline){$env:CL+=' /DASHBRINGER_STORAGE_BASELINE'}
 if($VerifyLiveData){$env:CL+=' /DASHBRINGER_STORAGE_LIVE'}
 $env:_LINK_="$previousLink /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{& "$repo/tools/tests/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/AshbringerStorageNative.cpp -ProbeName AshbringerStorageNative -OutputFile "$out/native.tsv" *> "$out/catalog.log"
  if(!$? -or $LASTEXITCODE){throw "Native Ashbringer storage regression failed: $out/catalog.log"}
 }}finally{Pop-Location}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$previousCompiler;$env:_LINK_=$previousLink}
if((Get-FileHash $configPath).Hash -ne $hash){throw 'Canonical configuration changed'}

foreach($path in $inputs){if((Get-FileHash $path).Hash -ne $hashes[$path]){throw 'Native Ashbringer storage source/final-link inputs changed during execution'}}
@{Inputs=$hashes;Probe=(Get-FileHash "$repo/build-extractors/runtime-test/legion-legendary-catalog/AshbringerStorageNative.exe").Hash;NativeReleaseNotAsan=$true;Baseline=[bool]$Baseline;VerifyLiveData=[bool]$VerifyLiveData}|ConvertTo-Json|Set-Content "$out/native-inputs.json"
Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|CASE|CONTROL|COUNTERFACTUAL)'}
