# Whole native spell fanout/clone AI/lifetime and retained actual capture.
# Earlier admission/pose/source clocks and returned-home endpoint controlled.
param([switch]$VerifyNativeCatalog,[switch]$RefuseVanish,[switch]$ResetBaseline)
if($RefuseVanish -and $ResetBaseline){throw 'Select separate baseline or refusal run'}
$Baseline=$false;$LiveData=$true

$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/devourer-clones-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
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
$pinned=(& git -C $repo show '47b792254:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp') -join "`n"
if($LASTEXITCODE){throw 'Pinned native door source unavailable'}
if($Baseline){$devourer=$pinned}else{
 . "$PSScriptRoot/DevourerEscapeCertificate.ps1"
 if(((Restore-DevourerEscapeBoundary $devourer)-replace '\s','') -cne ($pinned-replace '\s','')){throw 'Unreviewed change outside scoped door callbacks'}
}
if($ResetBaseline){$devourer=(& git -C $repo show 'c48f300b0:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned clone reset source missing'}}
$body=(Block $devourer 'class go_devourer_misc')+';';if(!$Baseline){$body=(Block $devourer 'namespace DevourerEscape')+(Block $devourer 'class spell_devourer_escape')+';'+$body}
$body=$body.Replace('class go_devourer_misc','class go_devourer_door_probe').Replace('go_devourer_misc()','go_devourer_door_probe()')
[IO.File]::WriteAllText("$out/DoorAI.inc",$body)
$instance=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/instance_devourer.cpp" -Raw
[IO.File]::WriteAllText("$out/DevourerInstance.inc",(Block $instance 'class instance_devourer :')+';')
$capture=(Block $devourer 'class npc_akaari_shadowgore :')+';'
$capture=$capture.Replace('class npc_akaari_shadowgore :','class npc_devourer_capture_probe :').Replace('npc_akaari_shadowgore() :','npc_devourer_capture_probe() :')
[IO.File]::WriteAllText("$out/Capture.inc",$capture)
$previous=$env:LEGION_DIALOGUE_CONFIG;$previousCompiler=$env:CL;$previousLink=$env:_LINK_
$inputs=@('src/server/game/Maps/Map.cpp','src/server/game/Entities/Creature/TemporarySummon.cpp','src/server/game/Entities/Creature/Creature.cpp','src/server/game/AI/CoreAI/CombatAI.cpp','src/server/game/AI/CreatureAISelector.cpp','tools/tests/DevourerClonesNative.cpp','tools/tests/Test-DevourerClonesNative.ps1','tools/research/DevourerClonesCatalog.cpp','src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp','tools/tests/DevourerEscapeNative.cpp','tools/tests/DevourerCaptureNative.cpp','tools/tests/Test-DevourerCaptureNative.ps1','src/common/Utilities/EventProcessor.cpp','src/common/Utilities/EventProcessor.h','build-extractors/src/common/Release/common.lib','src/server/game/Spells/Spell.cpp','src/server/game/Spells/SpellEffects.cpp','src/server/game/Spells/SpellScript.cpp','src/server/game/Entities/Unit/Unit.cpp','tools/tests/AltheaIntroNative.cpp','src/server/game/Entities/Item/Item.cpp','src/server/game/Entities/GameObject/GameObject.cpp','src/server/game/Globals/ObjectAccessor.cpp','src/server/game/Globals/ObjectAccessor.h','src/server/game/Entities/Player/Player.cpp','src/server/game/Entities/Object/QuestObjectScope.h','build-extractors/src/server/game/Release/game.lib','build-extractors/src/server/scripts/Release/scripts.lib')
$hashes=@{};foreach($path in $inputs){$hashes[$path]=(Get-FileHash "$repo/$path").Hash}
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$previousCompiler /DNDEBUG /Zi /I`"$out`" /I`"$repo/src/server/scripts/Legion`""
 if($LiveData){$env:CL+=' /DDEVOURER_ESCAPE_LIVE'}
 if($RefuseVanish){$env:CL+=' /DDEVOURER_CLONE_REFUSAL'}
 if($ResetBaseline){$env:CL+=' /DDEVOURER_CLONE_RESET_BASELINE'}
 $env:_LINK_="$previousLink /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{& "$repo/tools/tests/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  if($VerifyNativeCatalog){
   & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile DevourerClonesCatalog.cpp -ProbeName DevourerClonesCatalog -OutputFile "$out/metadata.tsv" *> "$out/metadata.log"
   if(!$? -or $LASTEXITCODE){throw "Native clone metadata failed: $out/metadata.log"}
  }
  $probeSource='../tests/DevourerClonesNative.cpp'
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile $probeSource -ProbeName DevourerClonesNative -OutputFile "$out/native.tsv" *> "$out/catalog.log"
  if(!$? -or $LASTEXITCODE){throw "Native capture failed: $out/catalog.log"}

 }}finally{Pop-Location}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$previousCompiler;$env:_LINK_=$previousLink}
if((Get-FileHash $configPath).Hash -ne $hash){throw 'Canonical configuration changed'}
foreach($path in $inputs){if((Get-FileHash "$repo/$path").Hash -ne $hashes[$path]){throw 'Native capture source/final-link inputs changed during execution'}}
@{Inputs=$hashes;Probe=(Get-FileHash "$repo/build-extractors/runtime-test/legion-legendary-catalog/DevourerClonesNative.exe").Hash;NativeReleaseNotAsan=$true}|ConvertTo-Json|Set-Content "$out/native-inputs.json"
Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|CASE|CONTROL|COUNTERFACTUAL)'}
