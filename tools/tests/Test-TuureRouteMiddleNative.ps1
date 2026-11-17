# Native Tuure route-middle conversation retry; no persistent character writes.
param([switch]$Baseline,[switch]$Faults)
if($Baseline -and $Faults){throw "Baseline and recovered faults are separate runs"}
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/tuure-route-middle-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
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
[IO.File]::WriteAllText("$out/NativeAltheaQuest.inc",$query+' " WHERE ID IN(40710,42504,42627,41993)"')
$configPath="$repo/build-extractors/bin/Release/worldserver.conf";$hash=(Get-FileHash $configPath).Hash;$config=Get-Content $configPath -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'));[IO.File]::WriteAllText("$out/worldserver.conf",$config)
[IO.File]::WriteAllText("$out/AltheaIntro.inc",(Block $source 'class npc_sister_althea_ebonlocke : public CreatureScript')+';')
$instanceSource=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/instance_kingslayers.cpp" -Raw
[IO.File]::WriteAllText("$out/KingslayersInstance.inc",(Block $instanceSource 'class instance_kingslayers : public InstanceMapScript')+';')
$instancePath='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/instance_silver_hand.cpp'
$instance=Get-Content "$repo/$instancePath" -Raw
if(!$instance){throw 'Actual/pinned shared instance source missing'}
[IO.File]::WriteAllText("$out/SilverHandInstance.inc",(Block $instance 'class instance_silver_hand : public InstanceMapScript')+';')
$text=[IO.File]::ReadAllText("$repo/src/server/scripts/Scenario/Artifacts/Priest/Tuure/Tuure.cpp")
[IO.File]::WriteAllText("$out/TuurePolicy.inc",(Block $text 'namespace TuureFirstFight'))
$instance=[IO.File]::ReadAllText("$repo/src/server/scripts/Scenario/Artifacts/Priest/Tuure/instance_tuure.cpp")
if($Baseline){$instance=(& git -C $repo show '42b10683c:src/server/scripts/Scenario/Artifacts/Priest/Tuure/instance_tuure.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned Tuure middle-route instance missing'}}
[IO.File]::WriteAllText("$out/TuureInstance.inc",$instance)
$previous=$env:LEGION_DIALOGUE_CONFIG;$previousCompiler=$env:CL;$previousLink=$env:_LINK_
$inputs=@("$repo/src/server/scripts/Scenario/Artifacts/Priest/Tuure/Tuure.cpp","$repo/src/server/scripts/Scenario/Artifacts/Priest/Tuure/instance_tuure.cpp","$repo/tools/tests/TuureRouteMiddleNative.cpp","$repo/tools/tests/Test-TuureRouteMiddleNative.ps1","$repo/src/server/game/Entities/Conversation/Conversation.cpp","$repo/src/server/game/Entities/Object/Object.cpp","$repo/src/server/game/Maps/Map.cpp","$repo/build-extractors/src/server/game/Release/game.lib","$repo/build-extractors/src/server/scripts/Release/scripts.lib")
if(Test-Path "$repo/src/server/scripts/Scenario/Artifacts/Priest/Tuure/tuure_route_middle.h"){$inputs+="$repo/src/server/scripts/Scenario/Artifacts/Priest/Tuure/tuure_route_middle.h"}
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup|Where-Object Condition -like '*Release|x64*'|Select-Object -First 1
$libs=$group.Link.AdditionalDependencies.Split(';')|Where-Object {$_ -and $_ -notlike '%*'}|ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$inputs+=@($libs|Where-Object {Test-Path -LiteralPath $_})
$inputs+=@(Get-ChildItem "$repo/src/server/scripts/Scenario/Artifacts/Priest/Tuure" -Filter "tuure_*.h" -File | ForEach-Object {$_.FullName})
$inputs+=@("$out/TuurePolicy.inc","$out/TuureInstance.inc","$repo/tools/tests/SlaghammerCompanionNative.cpp","$repo/tools/tests/AltheaIntroNative.cpp","$repo/src/server/scripts/Scenario/Artifacts/Priest/Tuure/Tuure.h")
# Reusable MSBuild workers can remain idle after completion. Certify the actual
# source and final-link library bytes before/after the sequential native run.
$hashes=@{};foreach($path in $inputs){$hashes[$path]=(Get-FileHash $path).Hash}
[IO.File]::WriteAllText("$out/native-before-inputs.json",($hashes|ConvertTo-Json))
Copy-Item "$PSScriptRoot/TuureRouteMiddleNative.cpp" "$out/ProbeSource.cpp"
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$previousCompiler /DNDEBUG /Zi /I`"$out`" /I`"$repo/src/server/scripts/Legion`" /I`"$repo/src/server/scripts/Scenario/Artifacts/Priest/Tuure`""


 $env:_LINK_="$previousLink /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 if($Baseline){$env:CL+=' /DTUURE_ROUTE_MIDDLE_BASELINE'}
 if($Faults){$env:CL+=' /DTUURE_ROUTE_MIDDLE_FAULTS'}
 Push-Location "$repo/build-extractors/bin/Release"
 try{try{& "$repo/tools/tests/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/TuureRouteMiddleNative.cpp -ProbeName TuureRouteMiddleNative -OutputFile "$out/native.tsv" *> "$out/catalog.log"
  if(!$? -or $LASTEXITCODE){throw "Native Tuure middle-route regression failed: $out/catalog.log"}
 }}catch{
  if(!$Faults){throw}
  $e=Get-Content "$out/evidence/diagnostics-after.json" -Raw|ConvertFrom-Json
  if(!$e.Completed -or @($e.ChangedCrashes).Count -or @($e.AppendedDBErrors).Count -or @($e.ServerDiagnostics).Count -ne 4 -or @($e.ServerDiagnostics|Where-Object {$_ -notmatch 'WARN.*Tuure route: dialogue2007 unavailable after thirty attempts; retain original-recipient/actor re-entry retry without delaying progression'}).Count){throw}
  'PASS reviewed exactly four expected exhausted-dialogue warnings; native faults recovered; no other runtime diagnostics'
 }}finally{Pop-Location}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$previousCompiler;$env:_LINK_=$previousLink}
if((Get-FileHash $configPath).Hash -ne $hash){throw 'Canonical configuration changed'}

foreach($path in $inputs){if((Get-FileHash $path).Hash -ne $hashes[$path]){throw 'Native Tuure middle-route source/final-link inputs changed during execution'}}
@{Inputs=$hashes;Probe=(Get-FileHash "$repo/build-extractors/runtime-test/legion-legendary-catalog/TuureRouteMiddleNative.exe").Hash;NativeReleaseNotAsan=$true;Baseline=[bool]$Baseline;Counterfactual=[bool]$Baseline;Faults=[bool]$Faults}|ConvertTo-Json|Set-Content "$out/native-inputs.json"
Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|CASE|CONTROL|COUNTERFACTUAL)'}
