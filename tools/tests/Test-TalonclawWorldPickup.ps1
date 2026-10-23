param([switch]$Baseline,[switch]$BaselineSummoner,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/talonclaw-world-pickup"
New-Item -ItemType Directory -Force $out | Out-Null
function Section([string]$source,[string]$start,[string]$end){
 $first=$source.IndexOf($start);$last=$source.IndexOf($end,$first)
 if($first -lt 0 -or $last -lt $first){throw "Missing production section $start"}
 $source.Substring($first,$last-$first)
}
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
[IO.File]::WriteAllText("$out/Active.inc",(Section $source 'bool Active(Player*' 'bool Use(Player*'))
[IO.File]::WriteAllText("$out/Retire.inc",(Section $source 'namespace TalonclawWorldPickup' 'class go_artifact_acquisition'))
if($Baseline){$source=(& git -C $repo show ff3c129a:src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp) -join "`n"}
$end=if($source.Contains('class go_marksmanship_pickup')){'class go_marksmanship_pickup'}else{'namespace TalonclawPresentation'}
[IO.File]::WriteAllText("$out/Binding.inc",(Section $source 'class go_artifact_acquisition' $end))
$source=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.h" -Raw
$members=@('ObjectGuid GetSummonerGUID() const { return m_summonerGUID; }','void SetSummonerGUID(ObjectGuid guid) { m_summonerGUID = guid; }','ObjectGuid m_summonerGUID = ObjectGuid::Empty;')
foreach($member in $members){if(!$source.Contains($member)){throw "Native provenance member missing: $member"}}
[IO.File]::WriteAllText("$out/Summoner.inc",$members -join "`n")
$source=if($BaselineSummoner){(& git -C $repo show ff3c129a:src/server/game/Entities/Object/Object.cpp) -join "`n"}else{Get-Content "$repo/src/server/game/Entities/Object/Object.cpp" -Raw}
[IO.File]::WriteAllText("$out/SummonGameObject.inc",(Section $source 'GameObject* WorldObject::SummonGameObject(uint32 entry, float x' 'Creature* WorldObject::SummonTrigger('))
$source=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
[IO.File]::WriteAllText("$out/SetLootState.inc",(Section $source 'void GameObject::SetLootState(' '    if (m_model)')+"}`n")
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
[IO.File]::WriteAllText("$out/OnStateChanged.inc",(Section $source 'void SmartGameObjectAI::OnStateChanged(' 'void SmartGameObjectAI::EventInform('))
$source=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/Despawn.inc",(Section $source 'void Creature::DespawnOrUnsummon(uint32' 'bool Creature::IsImmunedToSpell('))
$source=Get-Content "$repo/src/server/game/Entities/Creature/TemporarySummon.h" -Raw
[IO.File]::WriteAllText("$out/DelayEvent.inc",(Section $source 'class ForcedUnsummonDelayEvent : public BasicEvent' '#endif'))
$source=Get-Content "$repo/src/server/game/Entities/Creature/TemporarySummon.cpp" -Raw
[IO.File]::WriteAllText("$out/UnSummonDelay.inc",(Section $source 'void TempSummon::UnSummon(uint32 msTime)' '    // Warlock artifact trait')+"    CompleteUnSummon();`n}`n")
[IO.File]::WriteAllText("$out/DelayExecute.inc",(Section $source 'bool ForcedUnsummonDelayEvent::Execute(' 'void TempSummon::RemoveFromWorld()'))
$source=Get-Content "$repo/src/server/game/Scripting/ScriptMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/ScriptDispatch.inc",(Section $source 'void ScriptMgr::OnGameObjectLootStateChanged(' 'void ScriptMgr::OnGameObjectStateChanged('))
$source=Get-Content "$repo/src/server/game/Handlers/LootHandler.cpp" -Raw
$predicate=[regex]::Match($source,'else if \((loot->isLooted\(\) \|\| go->GetGoType\(\) == GAMEOBJECT_TYPE_FISHINGNODE)\)')
if(!$predicate.Success -or !$source.Contains('go->SetLootState(GO_JUST_DEACTIVATED, static_cast<Unit*>(player));')){throw 'Native real-loot completion admission changed'}
[IO.File]::WriteAllText("$out/CompletedLootPredicate.inc",'return '+$predicate.Groups[1].Value+';')
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/TalonclawWorldPickupTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native Talonclaw world-pickup regression failed'}
if($VerifyReleaseBinary){
 & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile GameObjectSummonLayout.cpp -ProbeName GameObjectSummonLayout -OutputFile "$out/layout.tsv"
 $previousPython=$env:PYTHONPATH
 try {
  $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython"
  & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_talonclaw_world_pickup_release.py" "$repo/build-extractors/bin/Release"
  if($LASTEXITCODE){throw 'Deployed world-pickup verification failed'}
 }finally{$env:PYTHONPATH=$previousPython}
}
