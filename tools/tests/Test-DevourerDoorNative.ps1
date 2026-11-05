# Whole production door/native object/receipt; seeded key/prerequisite boundary.
param([switch]$Baseline,[switch]$Faults)
if($Baseline -and $Faults){throw 'Select a separate pinned counterfactual or deliberate fault run'}
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/devourer-door-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
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
$pinned=(& git -C $repo show 'a8d3bb692:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp') -join "`n"
if($LASTEXITCODE){throw 'Pinned native door source unavailable'}
if($Baseline){$devourer=$pinned}else{
 . "$PSScriptRoot/DevourerDoorCertificate.ps1"
 if(((Restore-DevourerDoorBoundary $devourer)-replace '\s','') -cne ($pinned-replace '\s','')){throw 'Unreviewed change outside scoped door callbacks'}
}
. "$PSScriptRoot/DevourerEscapeCertificate.ps1"
$devourer=Restore-DevourerEscapeBoundary $devourer
$body=(Block $devourer 'class go_devourer_misc')+';'
$body=$body.Replace('class go_devourer_misc','class go_devourer_door_probe').Replace('go_devourer_misc()','go_devourer_door_probe()')
[IO.File]::WriteAllText("$out/DoorAI.inc",$body)
$instance=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/instance_devourer.cpp" -Raw
[IO.File]::WriteAllText("$out/DevourerInstance.inc",(Block $instance 'class instance_devourer :')+';')
$previous=$env:LEGION_DIALOGUE_CONFIG;$previousCompiler=$env:CL;$previousLink=$env:_LINK_
$inputs=@('src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp','tools/tests/DevourerDoorNative.cpp','tools/tests/DevourerDoorFaultNative.cpp','tools/tests/AltheaIntroNative.cpp','src/server/game/Entities/Item/Item.cpp','src/server/game/Entities/GameObject/GameObject.cpp','src/server/game/Globals/ObjectAccessor.cpp','src/server/game/Globals/ObjectAccessor.h','src/server/game/Entities/Player/Player.cpp','src/server/game/Entities/Object/QuestObjectScope.h','build-extractors/src/server/game/Release/game.lib','build-extractors/src/server/scripts/Release/scripts.lib')
$hashes=@{};foreach($path in $inputs){$hashes[$path]=(Get-FileHash "$repo/$path").Hash}
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$previousCompiler /DNDEBUG /Zi /I`"$out`" /I`"$repo/src/server/scripts/Legion`""
 if($Baseline){$env:CL+=' /DDEVOURER_DOOR_BASELINE'}
 $env:_LINK_="$previousLink /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{try{& "$repo/tools/tests/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  $probeSource=if($Faults){'../tests/DevourerDoorFaultNative.cpp'}else{'../tests/DevourerDoorNative.cpp'}
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile $probeSource -ProbeName DevourerDoorNative -OutputFile "$out/native.tsv" *> "$out/catalog.log"
  if(!$? -or $LASTEXITCODE){throw "Native door failed: $out/catalog.log"}
 }}catch{
  if(!$Faults -or $_.Exception.Message -notlike 'New runtime diagnostics require investigation:*'){throw}
  $d=Get-Content "$out/evidence/diagnostics-after.json" -Raw|ConvertFrom-Json
  if(!$d.Completed -or @($d.ChangedCrashes).Count -or @($d.AppendedDBErrors).Count -or @($d.ServerDiagnostics).Count -ne 4){throw 'Unexpected native door fault diagnostics'}
  $actors=0;$credits=0
  foreach($line in $d.ServerDiagnostics){if($line -like '*Devourer door: barrier249391 unavailable; retain Wardstone and50536 retry*'){++$actors}elseif($line -like '*Devourer door: native50536 not acknowledged; retain Wardstone and barrier for retry*'){++$credits}else{throw 'Unexplained native door diagnostic'}}
  if($actors -ne 2 -or $credits -ne 2){throw 'Unexpected native refusal warning types'}
  'EXPLICIT FAULT: exactly2missing-actor/2rejected-credit warnings, clean crash/DBErrors, restored native retry passes. Separate from ordinary clean run.'
 }}finally{Pop-Location}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$previousCompiler;$env:_LINK_=$previousLink}
if((Get-FileHash $configPath).Hash -ne $hash){throw 'Canonical configuration changed'}
foreach($path in $inputs){if((Get-FileHash "$repo/$path").Hash -ne $hashes[$path]){throw 'Native claim source/final-link inputs changed during execution'}}
@{Inputs=$hashes;Probe=(Get-FileHash "$repo/build-extractors/runtime-test/legion-legendary-catalog/DevourerDoorNative.exe").Hash;NativeReleaseNotAsan=$true}|ConvertTo-Json|Set-Content "$out/native-inputs.json"
Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|CASE|CONTROL|COUNTERFACTUAL)'}
