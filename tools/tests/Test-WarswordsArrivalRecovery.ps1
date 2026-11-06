param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-arrival-recovery/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Warrior/Warswords/warswords_arrival.h" "$out/Recovery.inc"
 Copy-Item "$repo/src/server/game/Entities/Object/QuestObjectScope.h" "$out/Scope.inc"
 foreach($headerName in @('MoveSpline.h','QuestObjectScope.h','ObjectGuid.h','Spell.h','SpellMgr.h')){[IO.File]::WriteAllText("$out/$headerName",'// Explicit typed controlled endpoint.')}
 [IO.File]::WriteAllText("$out/Contact.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'bool Player::CanContact('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/WarswordsArrivalRecoveryTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" *> "$out/test.log";if($LASTEXITCODE){Get-Content "$out/test.log" -Tail 15;throw 'Actual Fury arrival recovery ASAN failed'}
 Get-Content "$out/test.log"|Where-Object {$_ -match '^PASS'}
}
