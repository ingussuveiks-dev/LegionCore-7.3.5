param([switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/ragnvald-retry/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($repo,$out) -Test {
 function Shape([string]$s){([regex]::Replace($s,'(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw 'Unclosed source'};$s.Substring($a,$b-$a)}
 function Prior([string]$path){$s=(& git -C $repo show "2c6952ecf:$path") -join "`n";if($LASTEXITCODE){throw 'Complete pre-second-battle source missing'};$s}
 $path='src/server/scripts/Scenario/Artifacts/Warrior/Intro/wars_intro_scenario.cpp';$actor=Get-Content "$repo/$path" -Raw;$old=Prior $path
 $branch=Block $actor 'if (me->GetEntry() != 97167 || me->GetMapId() != 1500 || me->GetDBTableGUIDLow() != 361094)'
 $body=$branch.Substring($branch.IndexOf('{')+1);$body=$body.Substring(0,$body.LastIndexOf('}'))
 if((Shape $actor.Replace($branch,$body)) -cne (Shape $old)){throw 'Only original first Ragnvald1412 factory may move; every native battle/timer/other actor body retained'}
 $path='src/server/scripts/Scenario/Artifacts/Warrior/Intro/ritual_transition.h';$ritual=Get-Content "$repo/$path" -Raw;$oldRitual=Prior $path
 $a=$ritual.IndexOf('resumed = true; pending = false;');$from=$ritual.LastIndexOf('{',$a);$to=$ritual.IndexOf('}', $a)+1
 $normal=$ritual.Remove($from,$to-$from).Insert($from,'{ resumed = true; pending = false; members.clear(); ai->DoAction(true); return; }').Replace(', SecondBattleData = 93334','')
 if((Shape $normal) -cne (Shape $oldRitual)){throw 'Only consumed native ritual handoff may transfer its original participant snapshot before clear; all other receipt/cast/cleanup behavior retained'}
 $path='src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/instance_broken_shore.cpp';$instance=Get-Content "$repo/$path" -Raw;$oldInstance=Prior $path
 $normal=$instance.Replace('#include "../../Warrior/Intro/ragnvald_second_battle.h"','').Replace(', ragnvaldSecondBattle(new RagnvaldSecondBattle::State(map))','').Replace('std::unique_ptr<RagnvaldSecondBattle::State> ragnvaldSecondBattle;','').Replace('if (type == WarriorRitual::SecondBattleData) ragnvaldSecondBattle->Begin(warriorRitual, value);','').Replace(' ragnvaldSecondBattle->Update();','')
 if((Shape $normal) -cne (Shape $oldInstance)){throw 'Only appended independent speech owner/request/update allowed; all prior fields/shared Paladin callbacks retained'}
 if($ScopeOnly){'PASS exact scoped1412 exclusion, original consumed ritual participant transfer and appended instance owner; all other native actor/battle/ritual/Paladin bodies retained';return}
 & "$PSScriptRoot/Test-WarriorRitual.ps1" -ExportOnly -OutputDirectory $out
 if(!$?){throw 'Actual ritual/native host export failed'}
 [IO.File]::WriteAllText("$out/SecondBattle.inc",((Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Warrior/Intro/ragnvald_second_battle.h" -Raw) -replace '(?m)^#include[^\n]*',''))
 $timer=Get-Content "$repo/src/common/Utilities/Timer.h" -Raw
 [IO.File]::WriteAllText("$out/SecondBattleTimer.inc",(Block $timer 'inline uint32 getMSTimeDiff(')+(Block $timer 'inline uint32 GetMSTimeDiffToNow('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run-second-battle.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/WarriorSecondBattleTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run-second-battle.cmd";if($LASTEXITCODE){throw 'Actual second-battle controller ASAN failed'}
 if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$prior";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_ragnvald_second_battle_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Registered second-battle Release proof failed'}}finally{$env:PYTHONPATH=$prior}}
}
