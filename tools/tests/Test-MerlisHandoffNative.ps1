# Whole original Merlis death/native owner credit/source handoff; explicit pinned and fault modes.
param([switch]$Baseline,[switch]$Faults)
$ErrorActionPreference='Stop';if($Baseline -and $Faults){throw 'Select pinned baseline or repaired faults separately'};$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/merlis-handoff-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force "$out/logs"|Out-Null
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing actual $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
$source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp" -Raw
$ai=Block $source 'struct npc_sister_althea_ebonlockeAI : ScriptedAI'
foreach($pair in @(@('AltheaVictory','Player* GetVictoryPlayer('),@('Althea','void JustDied(Unit* who) override'))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $ai $pair[1]))}
[IO.File]::WriteAllText("$out/AltheaActor.inc",(Block $source 'class AltheaDeathConversation final : public Conversation')+';')
$old=(& git -C $repo show 'eb96dd4e6:src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned nearest-player baseline missing'}
# Shared bootstrap declares these pinned victory prerequisite helpers. They
# must compile, but this intro probe never invokes the old door/death hooks.
[IO.File]::WriteAllText("$out/AltheaDoor.inc",(Block (Block $old 'struct go_kingslayers_doorAI : public GameObjectAI') 'bool GossipHello('))
[IO.File]::WriteAllText("$out/AltheaBeforeCredit.inc",(Block (Block $old 'struct npc_sister_althea_ebonlockeAI : ScriptedAI') 'void JustDied(Unit* who) override'))
$quest=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
$query=[regex]::Match($quest,'(?s)void QuestDataStoreMgr::LoadQuests\(\).*?WorldDatabase.Query\(("SELECT ".*?" FROM quest_template")\);').Groups[1].Value
if(!$query){throw 'Actual native quest query missing'}
[IO.File]::WriteAllText("$out/NativeAltheaQuest.inc",$query+' " WHERE ID IN(42504,42627)"')
$configPath="$repo/build-extractors/bin/Release/worldserver.conf";$hash=(Get-FileHash $configPath).Hash;$config=Get-Content $configPath -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'));[IO.File]::WriteAllText("$out/worldserver.conf",$config)
[IO.File]::WriteAllText("$out/AltheaIntro.inc",(Block $source 'class npc_sister_althea_ebonlocke : public CreatureScript')+';')
$instanceSource=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/instance_kingslayers.cpp" -Raw
[IO.File]::WriteAllText("$out/KingslayersInstance.inc",(Block $instanceSource 'class instance_kingslayers : public InstanceMapScript')+';')
$path='src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp';$pinned=(& git -C $repo show "cf1f7a103:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Merlis handoff missing'}
$body=Block $(if($Baseline){$pinned}else{$source}) 'class npc_merlis_malagan : public CreatureScript'
$body=$body.Replace('class npc_merlis_malagan :','class npc_merlis_handoff_probe :').Replace('npc_merlis_malagan() :','npc_merlis_handoff_probe() :')
[IO.File]::WriteAllText("$out/MerlisHandoff.inc",$body+';')
$previous=$env:LEGION_DIALOGUE_CONFIG;$previousCompiler=$env:CL;$previousLink=$env:_LINK_
$inputs=@("$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp","$repo/tools/tests/AltheaIntroNative.cpp","$repo/tools/tests/MerlisHandoffNative.cpp","$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/instance_kingslayers.cpp","$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/KingslayersGuardProgress.h","$repo/src/server/game/Entities/Player/Player.cpp","$repo/build-extractors/src/server/game/Release/game.lib","$repo/build-extractors/src/server/scripts/Release/scripts.lib")
# Reusable MSBuild workers can remain idle after completion. Certify the actual
# source and final-link library bytes before/after the sequential native run.
$hashes=@{};foreach($path in $inputs){$hashes[$path]=(Get-FileHash $path).Hash}
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$previousCompiler /DNDEBUG /Zi /I`"$out`" /I`"$repo/src/server/scripts/Legion`""
 if($Baseline){$env:CL+=' /DMERLIS_HANDOFF_BASELINE'}elseif($Faults){$env:CL+=' /DMERLIS_HANDOFF_FAULTS'}
 $env:_LINK_="$previousLink /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{try{& "$repo/tools/tests/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/MerlisHandoffNative.cpp -ProbeName MerlisHandoffNative -OutputFile "$out/native.tsv" *> "$out/catalog.log"
  if(!$? -or $LASTEXITCODE){throw "Native Merlis handoff regression failed: $out/catalog.log"}
 }}catch{
  if((!$Baseline -and !$Faults) -or $_.Exception.Message -notlike 'New runtime diagnostics require investigation:*'){throw}
  $diagnostics=Get-Content "$out/evidence/diagnostics-after.json" -Raw|ConvertFrom-Json
  $rows=@(Get-Content "$out/logs/DBErrors.log")
  $expected=if($Baseline){6}else{24}
  if(!$diagnostics.Completed -or @($diagnostics.ChangedCrashes).Count -or @($diagnostics.AppendedDBErrors).Count -ne 1 -or $rows.Count -ne 6 -or @($diagnostics.ServerDiagnostics).Count -ne $expected){throw 'Unexpected handoff diagnostics; retain unresolved'}
  foreach($row in $rows){if($row -notmatch 'Creature::Create\(\): creature template \(guidlow: [0-9]+, entry: 108222\) does not exist\.'){throw 'Unexpected native SQL error'}}
  foreach($row in $diagnostics.ServerDiagnostics){if($row -notmatch 'Creature::Create\(\): creature template \(guidlow: [0-9]+, entry: 108222\) does not exist\.' -and ($Baseline -or $row -notlike '*Merlis outro: native108222 unavailable; retain old actor and bounded-rate retry*')){throw 'Unexpected source warning/error'}}
  'EXPLICIT FAULT:6 real missing108222 errors individually reviewed; current repair additionally18 bounded retry WARNs for create/AI recovery/cancellation. Counterfactual/fault separate from ordinary clean runs.'
 }}finally{Pop-Location}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$previousCompiler;$env:_LINK_=$previousLink}
if((Get-FileHash $configPath).Hash -ne $hash){throw 'Canonical configuration changed'}

foreach($path in $inputs){if((Get-FileHash $path).Hash -ne $hashes[$path]){throw 'Native Merlis handoff source/final-link inputs changed during execution'}}
@{Inputs=$hashes;Probe=(Get-FileHash "$repo/build-extractors/runtime-test/legion-legendary-catalog/MerlisHandoffNative.exe").Hash;NativeReleaseNotAsan=$true}|ConvertTo-Json|Set-Content "$out/native-inputs.json"
Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|CASE|CONTROL|COUNTERFACTUAL)'}
