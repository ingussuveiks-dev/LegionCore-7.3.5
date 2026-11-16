param([switch]$Baseline,[switch]$Faults)
$ErrorActionPreference='Stop'
if($Baseline -and $Faults){throw 'Choose one counterfactual or fault run'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/dreadblades-scout/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force "$out/logs" | Out-Null
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
$source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp" -Raw
$ai=Block $source 'struct npc_sister_althea_ebonlockeAI : ScriptedAI'
foreach($pair in @(@('AltheaVictory','Player* GetVictoryPlayer('),@('Althea','void JustDied(Unit* who) override'))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $ai $pair[1]))}
[IO.File]::WriteAllText("$out/AltheaActor.inc",(Block $source 'class AltheaDeathConversation final : public Conversation')+';')
$old=(& git -C $repo show 'eb96dd4e6:src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp') -join "`n"
if($LASTEXITCODE){throw 'Pinned bootstrap source absent'}
[IO.File]::WriteAllText("$out/AltheaDoor.inc",(Block (Block $old 'struct go_kingslayers_doorAI : public GameObjectAI') 'bool GossipHello('))
[IO.File]::WriteAllText("$out/AltheaBeforeCredit.inc",(Block (Block $old 'struct npc_sister_althea_ebonlockeAI : ScriptedAI') 'void JustDied(Unit* who) override'))
$quest=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
$query=[regex]::Match($quest,'(?s)void QuestDataStoreMgr::LoadQuests\(\).*?WorldDatabase.Query\(("SELECT ".*?" FROM quest_template")\);').Groups[1].Value
if(!$query){throw 'Native quest query absent'}
[IO.File]::WriteAllText("$out/NativeAltheaQuest.inc",$query+' " WHERE ID IN(42504,42627,40849)"')
$path='src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/Dreadblades.cpp'
$tethys=Get-Content "$repo/$path" -Raw
$pinned=(& git -C $repo show "1f7ffbe97:$path") -join "`n"
if($LASTEXITCODE){throw 'Pinned Scout source boundary absent'}
$starts=@('class npc_small_events_dreadblades: public CreatureScript','class npc_lord_brinebeard: public CreatureScript')
$surroundings=$tethys.Replace('#include "dreadblades_scout.h"','')
foreach($start in $starts){$surroundings=$surroundings.Replace((Block $tethys $start),(Block $pinned $start))}
if(($surroundings -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Unreviewed change outside scout/boss handoff classes'}
if($Baseline){$tethys=$pinned}
[IO.File]::WriteAllText("$out/Scout.inc",(($starts | ForEach-Object {(Block $tethys $_)+';'}) -join "`n"))
$configPath="$repo/build-extractors/bin/Release/worldserver.conf";$hash=(Get-FileHash $configPath).Hash
$config=[regex]::Replace((Get-Content $configPath -Raw),'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$prev=$env:LEGION_DIALOGUE_CONFIG;$prevCl=$env:CL;$prevLink=$env:_LINK_
$inputs=@("$repo/$path","$repo/src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/dreadblades_scout.h","$repo/tools/tests/DreadbladesScoutNative.cpp","$repo/build-extractors/src/server/game/Release/game.lib","$repo/build-extractors/src/server/scripts/Release/scripts.lib")
$hashes=@{};foreach($p in $inputs){$hashes[$p]=(Get-FileHash $p).Hash}
try {
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$prevCl /DNDEBUG /Zi /I`"$out`" /I`"$repo/src/server/scripts/Legion`""
 if($Baseline){$env:CL+=' /DSCOUT_BASELINE'}
 if($Faults){$env:CL+=' /DSCOUT_FAULTS'}
 $env:_LINK_="$prevLink /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try {
  try { & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
   & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/DreadbladesScoutNative.cpp -ProbeName DreadbladesScoutNative -OutputFile "$out/native.tsv" *> "$out/catalog.log"
   if(!$? -or $LASTEXITCODE){throw "Scout native check failed: $out/catalog.log"}
  }} catch {
   if(!$Faults -or $_.Exception.Message -notlike 'New runtime diagnostics require investigation:*'){throw}
   $d=Get-Content "$out/evidence/diagnostics-after.json" -Raw | ConvertFrom-Json
   if(!$d.Completed -or @($d.ChangedCrashes).Count -or @($d.AppendedDBErrors).Count -or @($d.ServerDiagnostics).Count -ne 10){throw 'Unexpected Scout fault diagnostics'}
   foreach($line in $d.ServerDiagnostics){if($line -notlike '*Dreadblades scout: original Brinebeard intro not acknowledged; retain same-source handoff retry*'){throw 'Unexplained Scout fault diagnostic'}}
   'EXPLICIT FAULT: ten expected missing/unavailable-original-boss warnings; each paired with clean same-actor recovery or fresh-attempt admission. No crash/DBError.'
  }
 } finally {Pop-Location}
} finally {$env:LEGION_DIALOGUE_CONFIG=$prev;$env:CL=$prevCl;$env:_LINK_=$prevLink}
if((Get-FileHash $configPath).Hash -ne $hash){throw 'Canonical config changed'}
foreach($p in $inputs){if((Get-FileHash $p).Hash -ne $hashes[$p]){throw 'Source/library changed during native check'}}
@{Inputs=$hashes;NativeReleaseNotAsan=$true;Probe=(Get-FileHash "$repo/build-extractors/runtime-test/legion-legendary-catalog/DreadbladesScoutNative.exe").Hash} | ConvertTo-Json -Depth 3 | Set-Content "$out/native-inputs.json"
Get-Content "$out/native.tsv" | Where-Object {$_ -match '^(PASS|COUNTERFACTUAL)'}
