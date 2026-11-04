# Whole production intro/native regression; run -Faults separately for explicit diagnostic injection.
param([switch]$Faults)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/althea-intro-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
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
$previous=$env:LEGION_DIALOGUE_CONFIG;$previousCompiler=$env:CL;$previousLink=$env:_LINK_
$inputs=@("$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp","$repo/tools/tests/AltheaIntroNative.cpp","$repo/build-extractors/src/server/game/Release/game.lib","$repo/build-extractors/src/server/scripts/Release/scripts.lib")
# Reusable MSBuild workers can remain idle after completion. Certify the actual
# source and final-link library bytes before/after the sequential native run.
$hashes=@{};foreach($path in $inputs){$hashes[$path]=(Get-FileHash $path).Hash}
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$previousCompiler /DNDEBUG /Zi /I`"$out`" /I`"$repo/src/server/scripts/Legion`""
 if($Faults){$env:CL+=' /DALTHEA_INTRO_FAULTS'}
 $env:_LINK_="$previousLink /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{try{& "$repo/tools/tests/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/AltheaIntroNative.cpp -ProbeName AltheaIntroNative -OutputFile "$out/native.tsv" *> "$out/catalog.log"
  if(!$? -or $LASTEXITCODE){throw "Native Althea intro failed: $out/catalog.log"}
 }}catch{
  if(!$Faults -or $_.Exception.Message -notlike 'New runtime diagnostics require investigation:*'){throw}
  $diagnostics=Get-Content "$out/evidence/diagnostics-after.json" -Raw|ConvertFrom-Json
  if(!$diagnostics.Completed -or @($diagnostics.ChangedCrashes).Count -or @($diagnostics.AppendedDBErrors).Count -or @($diagnostics.ServerDiagnostics).Count -ne 8){throw 'Injected native failure has unexpected diagnostics; retain unresolved'}
  foreach($id in @('2422','2424','2425','native51549')){
   $pattern=if($id -eq 'native51549'){'Althea intro: native51549 not acknowledged; retain bounded-rate progression retry'}else{"Althea intro: conversation $id unavailable; retain bounded-rate presentation retry independently of progression"}
   if(@($diagnostics.ServerDiagnostics|Where-Object {$_ -match [regex]::Escape($pattern)}).Count -ne 2){throw 'Injected native warning count/type differs; retain unresolved'}
  }
  [IO.File]::WriteAllText("$out/expected-diagnostics-fault.txt",'EXPLICIT FAULT INJECTION: diagnostic wrapper correctly rejected eight WARNs (two each2422/2424/2425/native51549). Each injected endpoint recovered in its native case; no crash/DBErrors/other server diagnostic. This is separate from the clean ordinary regression.')
  Get-Content "$out/expected-diagnostics-fault.txt"
 }}finally{Pop-Location}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$previousCompiler;$env:_LINK_=$previousLink}
if((Get-FileHash $configPath).Hash -ne $hash){throw 'Canonical configuration changed'}
if((Test-Path "$out/logs/DBErrors.log") -and (Get-Item "$out/logs/DBErrors.log").Length){throw 'Native intro DBErrors require investigation'}
foreach($path in $inputs){if((Get-FileHash $path).Hash -ne $hashes[$path]){throw 'Native intro source/final-link inputs changed during execution'}}
@{Inputs=$hashes;Probe=(Get-FileHash "$repo/build-extractors/runtime-test/legion-legendary-catalog/AltheaIntroNative.exe").Hash;NativeReleaseNotAsan=$true}|ConvertTo-Json|Set-Content "$out/native-inputs.json"
Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|CASE)'}
