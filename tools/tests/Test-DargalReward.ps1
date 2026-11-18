param([switch]$Baseline,[switch]$AddressSanitizer,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/dargal-reward/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Path $out -Force|Out-Null
$source=if($Baseline){(& git -C $repo show '3b74d381e:src/server/game/Entities/Player/Player.cpp') -join "`n"}else{Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw}
$marker='bool Player::CanRewardQuest(Quest const* quest, bool msg)';$a=$source.IndexOf($marker);$b=$source.IndexOf('bool Player::CanRewardQuest(Quest const* quest, uint32 reward, bool msg)',$a)
if($a -lt 0 -or $b -le $a){throw 'Native reward methods missing'}
[IO.File]::WriteAllText("$out/CanReward.inc",$source.Substring($a,$b-$a))
if($source.Substring($b) -notmatch '(?s)^bool Player::CanRewardQuest\(Quest const\* quest, uint32 reward, bool msg\)\s*\{\s*//[^\r\n]*\s*if \(!CanRewardQuest\(quest, msg\)\)\s*return false;'){throw 'Inventory reward overload bypasses native base admission'}
if((Get-Content "$repo/src/server/game/Handlers/QuestHandler.cpp" -Raw) -notmatch '_player->CanRewardQuest\(quest, packet.ItemChoiceID, true\)'){throw 'Real quest reward packet admission changed'}
$data=Get-Content "$PSScriptRoot/DargalProgressionExpected.json" -Raw|ConvertFrom-Json
if(@($data.quest_objectives|Where-Object{$_.ID -eq 284748 -and $_.ObjectID -eq 108815 -and $_.Amount -eq 9}).Count -ne 1 -or @($data.quest_objectives|Where-Object{$_.ID -eq 285316 -and $_.ObjectID -eq 110635 -and $_.Amount -eq 1}).Count -ne 1){throw 'Matching native required9+1 data differs'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$extra=if($Baseline){'/DEXPECT_BASELINE '}else{''};if($AddressSanitizer){$extra+='/fsanitize=address /Zi /Fd"'+$out+'/test.pdb" '}
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $extra/I`"$out`" `"$PSScriptRoot/DargalRewardTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Dargal reward regression failed'}}
if($VerifyReleaseBinary){$prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps";& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/pe-evidence" -Test {& 'C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -X utf8 "$PSScriptRoot/verify_dargal_reward_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical reward admission not linked'}}}finally{$env:PYTHONPATH=$prior}}