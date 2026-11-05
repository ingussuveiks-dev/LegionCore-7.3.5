# Native final Akaari1942 owner/caster/actor; real Map/Player/Pet/Guardian/Conversation, seeded combat flag.
param([switch]$Baseline,[switch]$Faults)
if($Baseline -and $Faults){throw 'Counterfactual and injected faults are separate'}



$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/devourer-opening-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force "$out/logs"|Out-Null
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing actual $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
$source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp" -Raw
$ai=Block $source 'struct npc_sister_althea_ebonlockeAI : ScriptedAI'
foreach($pair in @(@('AltheaVictory','Player* GetVictoryPlayer('),@('Althea','void JustDied(Unit* who) override'))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $ai $pair[1]))}
[IO.File]::WriteAllText("$out/AltheaActor.inc",(Block $source 'class AltheaDeathConversation final : public Conversation')+';')
$old=(& git -C $repo show 'eb96dd4e6:src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned nearest-player baseline missing'}
# Shared bootstrap declares these pinned victory prerequisite helpers. They
# must compile, but this probe never invokes the old door/death hooks.
[IO.File]::WriteAllText("$out/AltheaDoor.inc",(Block (Block $old 'struct go_kingslayers_doorAI : public GameObjectAI') 'bool GossipHello('))
[IO.File]::WriteAllText("$out/AltheaBeforeCredit.inc",(Block (Block $old 'struct npc_sister_althea_ebonlockeAI : ScriptedAI') 'void JustDied(Unit* who) override'))
$quest=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
$query=[regex]::Match($quest,'(?s)void QuestDataStoreMgr::LoadQuests\(\).*?WorldDatabase.Query\(("SELECT ".*?" FROM quest_template")\);').Groups[1].Value
if(!$query){throw 'Actual native quest query missing'}
[IO.File]::WriteAllText("$out/NativeAltheaQuest.inc",$query+' " WHERE ID IN(42504,42627,41924)"')
$configPath="$repo/build-extractors/bin/Release/worldserver.conf";$hash=(Get-FileHash $configPath).Hash;$config=Get-Content $configPath -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'));[IO.File]::WriteAllText("$out/worldserver.conf",$config)
[IO.File]::WriteAllText("$out/AltheaIntro.inc",(Block $source 'class npc_sister_althea_ebonlocke : public CreatureScript')+';')
$instanceSource=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/instance_kingslayers.cpp" -Raw
[IO.File]::WriteAllText("$out/KingslayersInstance.inc",(Block $instanceSource 'class instance_kingslayers : public InstanceMapScript')+';')
$devourer=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp" -Raw
if(!$Baseline){
 . "$PSScriptRoot/DevourerOpeningCertificate.ps1"
 [void](Restore-DevourerOpeningBoundary $devourer)
}
if($Baseline){$devourer=(& git -C $repo show 'c000a548d:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned opening source missing'}}
$body=Block $devourer 'class npc_akaari_shadowgore_last : public CreatureScript'
$body=$body.Replace('class npc_akaari_shadowgore_last :','class npc_akaari_last_probe :').Replace('npc_akaari_shadowgore_last() :','npc_akaari_last_probe() :')
[IO.File]::WriteAllText("$out/Entrance.inc",$body+';')
$instance=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/instance_devourer.cpp" -Raw
[IO.File]::WriteAllText("$out/DevourerInstance.inc",(Block $instance 'class instance_devourer :')+';')
$previous=$env:LEGION_DIALOGUE_CONFIG;$previousCompiler=$env:CL;$previousLink=$env:_LINK_
$inputs=@("$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp","$repo/tools/tests/AltheaIntroNative.cpp","$repo/tools/tests/DevourerOpeningNative.cpp","$repo/src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp","$repo/src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/instance_devourer.cpp","$repo/src/server/game/Entities/Object/Object.cpp","$repo/src/server/game/Grids/Notifiers/GridNotifiers.cpp","$repo/src/server/game/Grids/Notifiers/GridNotifiers.h","$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/instance_kingslayers.cpp","$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/KingslayersGuardProgress.h","$repo/src/server/game/Entities/Player/Player.cpp","$repo/build-extractors/src/server/game/Release/game.lib","$repo/build-extractors/src/server/scripts/Release/scripts.lib")
# Reusable MSBuild workers can remain idle after completion. Certify the actual
# source and final-link library bytes before/after the sequential native run.
$hashes=@{};foreach($path in $inputs){$hashes[$path]=(Get-FileHash $path).Hash}
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$previousCompiler /DNDEBUG /Zi /I`"$out`" /I`"$repo/src/server/scripts/Legion`""
 if($Baseline){$env:CL+=' /DDEVOURER_OPENING_BASELINE'}
 if($Faults){$env:CL+=' /DDEVOURER_OPENING_FAULTS'}



 $env:_LINK_="$previousLink /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{try{& "$repo/tools/tests/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/DevourerOpeningNative.cpp -ProbeName DevourerOpeningNative -OutputFile "$out/native.tsv" *> "$out/catalog.log"
  if(!$? -or $LASTEXITCODE){throw "Native Devourer opening failed: $out/catalog.log"}
 }}catch{
  if(!$Faults -or $_.Exception.Message -notlike 'New runtime diagnostics require investigation:*'){throw}
  $d=Get-Content "$out/evidence/diagnostics-after.json" -Raw|ConvertFrom-Json
  if(!$d.Completed -or @($d.ChangedCrashes).Count -or @($d.AppendedDBErrors).Count -or @($d.ServerDiagnostics).Count -ne 6){throw 'Unexpected opening fault diagnostics'}
  $factory=0;$recipient=0
  foreach($line in $d.ServerDiagnostics){if($line -like '*Akaari combat: conversation 1942 unavailable; retain bounded-rate same-line retry independently of combat*'){++$factory}elseif($line -like '*Akaari opening: original recipient unavailable; retain same-owner retry*'){++$recipient}else{throw 'Unexplained opening fault diagnostic'}}
  if($factory -ne 4 -or $recipient -ne 2){throw 'Unexpected opening warning types'}
  'EXPLICIT FAULT: exactly4factory/2original-recipient warnings reviewed; missing data/owner distance recovers, abandoned attempt is dropped. Separate from ordinary clean runs.'
 }}finally{Pop-Location}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$previousCompiler;$env:_LINK_=$previousLink}
if((Get-FileHash $configPath).Hash -ne $hash){throw 'Canonical configuration changed'}

foreach($path in $inputs){if((Get-FileHash $path).Hash -ne $hashes[$path]){throw 'Native Devourer opening source/final-link inputs changed during execution'}}
@{Inputs=$hashes;Probe=(Get-FileHash "$repo/build-extractors/runtime-test/legion-legendary-catalog/DevourerOpeningNative.exe").Hash;NativeReleaseNotAsan=$true}|ConvertTo-Json|Set-Content "$out/native-inputs.json"
Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|CASE|CONTROL|COUNTERFACTUAL)'}
