param([switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/hait-defeat/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($repo,$out) -Test {
 function Shape([string]$s){(($s -replace '(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw 'Unclosed source'};$s.Substring($a,$b-$a)}
 function Prior([string]$path){$s=(& git -C $repo show "8314d0379:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned complete pre-defeat source missing'};$s}
 $actorPath='src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_intro_scenario.cpp';$actor=Get-Content "$repo/$actorPath" -Raw;$oldActor=Prior $actorPath
 $normal=$actor.Replace('if (MonkHaitOpening::Source(me)) return; // Actual instance CreatureDies owns the first Demon.','').Replace('if (MonkHaitOpening::Source(me) && type == POINT_MOTION_TYPE && id == 1) return;','')
 if((Shape $normal) -cne (Shape $oldActor)){throw 'Only actual first-Hait defeat/point1 scoped exclusions permitted; all shared/unscoped actors retained'}
 $instancePath='src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/instance_class_art_scenarios.cpp';$instance=Get-Content "$repo/$instancePath" -Raw;$oldInstance=Prior $instancePath
 if((Shape $instance.Replace('haitOpening->Death(actor, killer); ','')) -cne (Shape $oldInstance)){throw 'Only original native CreatureDies handoff may change in complete shared instance'}
 $headerPath='src/server/scripts/Scenario/Artifacts/Monk/Intro/hait_opening.h';$header=Get-Content "$repo/$headerPath" -Raw;$oldHeader=Prior $headerPath
 $normal=$header.Replace('#include "MoveSpline.h"','').Replace('if (defeated) { UpdateDefeat(); return; }','')
 $from=$normal.IndexOf('MovementGenerator* defeatPoint');$to=$normal.IndexOf('Scenario* Progress() const',$from)
 if($from -lt 0 -or $to -lt 0){throw 'Appended defeat fields missing'};$normal=$normal.Remove($from,$to-$from)
 foreach($method in @('bool Victory() const','bool DefeatValid() const','Player* Contact(','void Death(','void UpdateDefeat()')){$normal=$normal.Replace((Block $header $method),'')}
 if((Shape $normal) -cne (Shape $oldHeader)){throw 'Original complete accepted opening/receipts/child clocks/friendly protection must remain unchanged'}
 if($ScopeOnly){'PASS exact original opening prefix retained; only appended genuine-death continuation/native instance handoff and first-Hait legacy exclusions; entire other actor/instance payload retained';return}
 & "$PSScriptRoot/Test-HaitOpening.ps1" -ExportOnly -OutputDirectory $out
 if(!$?){throw 'Actual current actor/instance/native host export failed'}
 [IO.File]::WriteAllText("$out/BeforeDefeatOpening.inc",($oldHeader -replace '(?m)^#include[^\n]*','').Replace('namespace MonkHaitOpening','namespace MonkHaitOpeningBeforeDefeat'))
 [IO.File]::WriteAllText("$out/BeforeDefeatActor.inc",(Block $oldActor 'class npc_master_hait :').Replace('npc_master_hait','npc_master_hait_before_defeat').Replace('MonkHaitOpening::','MonkHaitOpeningBeforeDefeat::')+';')
 [IO.File]::WriteAllText("$out/BeforeDefeatInstance.inc",(Block $oldInstance 'struct instance_monk_intro :').Replace('instance_monk_intro','instance_monk_intro_before_defeat').Replace('MonkHaitOpening::','MonkHaitOpeningBeforeDefeat::')+';')
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run-defeat.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/HaitDefeatTest.cpp`" /Fe`"$out/defeat.exe`" /Fo`"$out/defeat.obj`" /Fd`"$out/defeat.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/defeat.exe`"`nexit /b %errorlevel%`n")
 & "$out/run-defeat.cmd";$code=$LASTEXITCODE;[IO.File]::WriteAllText("$out/native-result.json",(@{Exit=$code}|ConvertTo-Json));if($code){throw 'Actual Hait defeat/native lifecycle ASAN failed'}
 if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$prior";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_hait_defeat_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Registered defeat Release proof failed'}}finally{$env:PYTHONPATH=$prior}}
}
