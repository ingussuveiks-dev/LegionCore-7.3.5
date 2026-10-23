param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-acquisition"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$s=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
$a=$s.IndexOf('namespace ArtifactAcquisition');$b=$s.IndexOf('namespace DarkRidersCompletion',$a)
[IO.File]::WriteAllText("$out/Acquisition.inc",$s.Substring($a,$b-$a))
$s=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$a=$s.IndexOf('void GameObject::SetLootState(');$b=$s.IndexOf('    if (m_model)',$a)
[IO.File]::WriteAllText("$out/SetLootState.inc",$s.Substring($a,$b-$a)+"}`n")
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
$a=$s.IndexOf('void SmartGameObjectAI::OnStateChanged(');$b=$s.IndexOf('void SmartGameObjectAI::EventInform(',$a)
[IO.File]::WriteAllText("$out/OnStateChanged.inc",$s.Substring($a,$b-$a))
$s=Get-Content "$repo/src/server/game/Handlers/LootHandler.cpp" -Raw
if(!$s.Contains('else if (loot->isLooted() || go->GetGoType() == GAMEOBJECT_TYPE_FISHINGNODE)') -or !$s.Contains('go->SetLootState(GO_JUST_DEACTIVATED, static_cast<Unit*>(player));')){throw 'Native completed-loot admission changed'}
$lootPredicate=[regex]::Match($s,'else if \((loot->isLooted\(\) \|\| go->GetGoType\(\) == GAMEOBJECT_TYPE_FISHINGNODE)\)')
[IO.File]::WriteAllText("$out/CompletedLootPredicate.inc","return "+$lootPredicate.Groups[1].Value+";")
$nativePlayer=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
if(!$nativePlayer.Contains('UpdateAchievementCriteria(CRITERIA_TYPE_OWN_ITEM, itemID, 1);')){throw 'Native new-item ownership event changed'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/ArtifactAcquisitionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Artifact acquisition native handler regression failed'}
if($VerifyReleaseBinary){
 $ascii=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe"))
 if(!$ascii.Contains('Artifact acquisition: player %u used owned artifact %u at object %u')){throw 'Canonical Release lacks artifact ownership repair'}
 $bytes=[IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe")
 # The native Pickup array is five adjacent uint32 fields. Verify the actual
 # linked quest tuple, not merely a log string shared with the stale handler.
 $hex=[BitConverter]::ToString($bytes)
 $correct=[BitConverter]::ToString([byte[]](@(249812,128861,3,42185,0) | ForEach-Object {[BitConverter]::GetBytes([uint32]$_)}))
 $stale=[BitConverter]::ToString([byte[]](@(249812,128861,3,42158,41574) | ForEach-Object {[BitConverter]::GetBytes([uint32]$_)}))
 if(!$hex.Contains($correct) -or $hex.Contains($stale)){throw 'Canonical Release has stale Hunter artifact quest admission'}
 'PASS: canonical executable contains artifact acquisition and the native Hunter42185 pickup tuple, without the stale prerequisite tuple'
}
