param([switch]$VerifyLiveData,[switch]$AddressSanitizer)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/twinblades-artifact-loot"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/twinblades-loot-data") -Test {
 & "$PSScriptRoot/Test-TwinbladesArtifactLootData.ps1" -VerifyLiveData:$VerifyLiveData
 $rows=Get-Content "$repo/build-extractors/runtime-test/twinblades-loot-data/temp-rows.tsv"
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
function Section([string]$text,[string]$start,[string]$end){$a=$text.IndexOf($start);$b=$text.IndexOf($end,[Math]::Max(0,$a));if($a -lt 0 -or $b -le $a){throw "Missing native $start"};$text.Substring($a,$b-$a)}
$cases=(Section $source '        case CONDITION_ITEM:' '        case CONDITION_ITEM_EQUIPPED:')+(Section $source '        case CONDITION_CLASS:' '        case CONDITION_RACE:')+(Section $source '        case CONDITION_QUESTREWARDED:' '        case CONDITION_QUEST_NONE:')+(Section $source '        case CONDITION_MAPID:' '        case CONDITION_AREAID:')+(Section $source '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:')+(Section $source '        case CONDITION_ALIVE:' '        case CONDITION_HP_VAL:')
[IO.File]::WriteAllText("$out/Conditions.inc",$cases)
[IO.File]::WriteAllText("$out/ElseGroups.inc",(Section $source 'bool ConditionMgr::IsObjectMeetToConditionList(' 'bool ConditionMgr::IsObjectMeetToConditions(WorldObject* object,'))
[IO.File]::WriteAllText("$out/SmartEventConditions.inc",(Section $source 'bool ConditionMgr::IsObjectMeetingSmartEventConditions(' 'bool Condition::isLoaded() const'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/RootDispatch.inc",(Section $source 'void SmartScript::ProcessEventsFor(' 'void SmartScript::ProcessAction('))
$source=Get-Content "$repo/src/server/game/Loot/LootMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/LootCondition.inc",(Section $source 'bool LootItem::AllowedForPlayer(Player const* player) const' 'void LootItem::AddAllowedLooter('))
$tail=Section $source '    if (item && !AllowedForPlayer(player,' 'uint32 Loot::GetMaxSlotInLootFor('
[IO.File]::WriteAllText("$out/SlotRecheck.inc",$tail.Substring(0,$tail.LastIndexOf('}')))
$source=Get-Content "$repo/src/server/game/Entities/GameObject/GameObjectData.cpp" -Raw
[IO.File]::WriteAllText("$out/GetLootId.inc",(Section $source 'uint32 GameObjectTemplate::GetLootId() const' 'uint32 GameObjectTemplate::GetGossipMenuId() const'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($mode in @('loot','root','gossip')){
 $kind=if($mode -eq 'loot'){4}else{22};$group=if($mode -eq 'loot'){251878}elseif($mode -eq 'root'){1}else{3}
 $rules=@($rows|Where-Object {$_ -like "rule`t$kind`t$group`t*"}|ForEach-Object {($_.Split("`t")|Select-Object -Skip 3) -join "`t"})
 if($rules.Count -ne $(if($mode -eq 'loot'){28}else{56})){throw "Wrong native rule count for $mode"}
 [IO.File]::WriteAllLines("$out/$mode.tsv",[string[]]$rules)
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/TalonclawPickupAdmissionTest.cpp`" /Fe`"$out/$mode.exe`" /Fo`"$out/$mode.obj`" /Fd`"$out/$mode.pdb`" /link /INCREMENTAL:NO`nif not %errorlevel%==0 exit /b %errorlevel%`n`"$out/$mode.exe`" `"$out/$mode.tsv`" twinblades-$mode 251878`nif not %errorlevel%==0 exit /b %errorlevel%`n`"$out/$mode.exe`" `"$out/$mode.tsv`" twinblades-alt-$mode 251878`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw "Native Twinblades $mode failed"}
}
 'PASS native actual loot selector/conditions/cached slots/SmartAI root and owned gossip for39247 and41119; no client/equip/render certification'
}
