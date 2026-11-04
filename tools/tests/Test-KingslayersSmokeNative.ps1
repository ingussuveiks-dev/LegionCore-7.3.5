# Actual Smoke Bomb SpellScript with native cast/effect admission and receipts; controlled earlier state and pinned counterfactual.
param([switch]$Baseline)


$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/kingslayers-smoke-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force "$out/logs"|Out-Null
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing actual $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
$source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp" -Raw
$ai=Block $source 'struct npc_sister_althea_ebonlockeAI : ScriptedAI'
foreach($pair in @(@('AltheaVictory','Player* GetVictoryPlayer('),@('Althea','void JustDied(Unit* who) override'))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $ai $pair[1]))}
[IO.File]::WriteAllText("$out/AltheaActor.inc",(Block $source 'class AltheaDeathConversation final : public Conversation')+';')
$old=(& git -C $repo show 'eb96dd4e6:src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned nearest-player baseline missing'}
# Shared bootstrap declares these pinned victory prerequisite helpers. They
# must compile, but this smoke probe never invokes the old door/death hooks.
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
$selected=$source
if($Baseline){$selected=(& git -C $repo show '81c43cc8c:src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned gate-order source missing'}}
$body=Block $selected 'class spell_kingslayers_smoke_bomb : public SpellScriptLoader'
$body=$body.Replace('class spell_kingslayers_smoke_bomb :','class spell_kingslayers_smoke_probe :').Replace('spell_kingslayers_smoke_bomb() :','spell_kingslayers_smoke_probe() :')
[IO.File]::WriteAllText("$out/Smoke.inc",$body+';')
$previous=$env:LEGION_DIALOGUE_CONFIG;$previousCompiler=$env:CL;$previousLink=$env:_LINK_
$inputs=@("$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp","$repo/tools/tests/AltheaIntroNative.cpp","$repo/tools/tests/KingslayersSmokeNative.cpp","$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/instance_kingslayers.cpp","$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/KingslayersGuardProgress.h","$repo/src/server/game/Entities/Player/Player.cpp","$repo/build-extractors/src/server/game/Release/game.lib","$repo/build-extractors/src/server/scripts/Release/scripts.lib")
# Reusable MSBuild workers can remain idle after completion. Certify the actual
# source and final-link library bytes before/after the sequential native run.
$hashes=@{};foreach($path in $inputs){$hashes[$path]=(Get-FileHash $path).Hash}
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$previousCompiler /DNDEBUG /Zi /I`"$out`" /I`"$repo/src/server/scripts/Legion`""
 if($Baseline){$env:CL+=' /DKINGSLAYERS_SMOKE_BASELINE'}


 $env:_LINK_="$previousLink /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{& "$repo/tools/tests/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/KingslayersSmokeNative.cpp -ProbeName KingslayersSmokeNative -OutputFile "$out/native.tsv" *> "$out/catalog.log"
  if(!$? -or $LASTEXITCODE){throw "Native smoke failed: $out/catalog.log"}
 }}finally{Pop-Location}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$previousCompiler;$env:_LINK_=$previousLink}
if((Get-FileHash $configPath).Hash -ne $hash){throw 'Canonical configuration changed'}

foreach($path in $inputs){if((Get-FileHash $path).Hash -ne $hashes[$path]){throw 'Native smoke source/final-link inputs changed during execution'}}
@{Inputs=$hashes;Probe=(Get-FileHash "$repo/build-extractors/runtime-test/legion-legendary-catalog/KingslayersSmokeNative.exe").Hash;NativeReleaseNotAsan=$true}|ConvertTo-Json|Set-Content "$out/native-inputs.json"
Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|CASE|CONTROL|COUNTERFACTUAL)'}
