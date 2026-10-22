$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/quest-details-delivery"
New-Item -ItemType Directory -Force $out | Out-Null
$source=Get-Content "$repo/src/server/game/Entities/Creature/GossipDef.cpp" -Raw
$start=$source.IndexOf('void PlayerMenu::SendQuestGiverQuestDetails(')
$end=$source.IndexOf('void PlayerMenu::SendQuestQueryResponse(',$start)
$method=$source.Substring($start,$end-$start)
$start=$method.IndexOf('    _session->SendPacket(')
if($start -lt 0){$start=$method.IndexOf('    player->SendDirectMessage(')}
[IO.File]::WriteAllText("$out/Delivery.inc",$method.Substring($start,$method.LastIndexOf('}')-$start))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/QuestDetailsDeliveryTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif %errorlevel% neq 0 exit /b %errorlevel%`n`"$out/test.exe`"`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Quest details must serialize and dispatch exactly once'}
