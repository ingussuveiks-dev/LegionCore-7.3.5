param([switch]$ScopeOnly,[switch]$ExportOnly,[string]$OutputDirectory='')
. "$PSScriptRoot/HaitOpeningPredecessor.ps1"
. "$PSScriptRoot/ChenEscortSpeechPredecessor.ps1"
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/monk-chen-early"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence/$(Get-Date -Format yyyyMMdd-HHmmss-fff)" -RuntimeDirectories @($repo,$out) -Test {
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
 function Shape([string]$s){(($s -replace '(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
 function Prior([string]$path){$s=(& git -C $repo show "8d66b5e09:$path") -join "`n";if($LASTEXITCODE){throw "Pinned early presentation predecessor missing: $path"};$s}
 $path='src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_intro_scenario.cpp';$source=Get-Content "$repo/$path" -Raw;$source=Get-HaitPreOpeningSource $path $source;$prior=Prior $path
 if($source.Contains('if (!MonkChenRescue::Source(me)) _presentationFunctions.AddDelayedEvent(4443,')){
  & "$PSScriptRoot/Test-ChenOpeningVoice.ps1" -ScopeOnly
  $source=(& git -C $repo show "8b9ea7da4:$path") -join "`n";if($LASTEXITCODE){throw 'Certified pre-voice actor missing'}
 }
 $hasNine=$source.Contains('#include "nine_czi_arrival.h"')
 if($hasNine){& "$PSScriptRoot/Test-MonkNineArrival.ps1" -ScopeOnly;$source=(& git -C $repo show "f67c49e3b:$path") -join "`n";if($LASTEXITCODE){throw 'Certified pre-Nine actor missing'}}
 $actor=Block $source 'class npc_monk_chen_porter :';$oldActor=Block $prior 'class npc_monk_chen_porter :';$normal=$actor
 foreach($deadline in @(4000,5000,8000)){
  $prefix="if (!MonkChenRescue::Source(me)) _presentationFunctions.AddDelayedEvent($deadline,"
  if(([regex]::Matches($actor,[regex]::Escape($prefix))).Count -ne 1){throw 'Original4/5 callbacks and redundant8s wrapper must be scoped exactly once'}
  $normal=$normal.Replace($prefix,"_presentationFunctions.AddDelayedEvent($deadline,")
 }
 if((Shape $normal) -cne (Shape $oldActor) -or (Shape $source.Replace($actor,$oldActor)) -cne (Shape $prior)){throw 'Other original actor, late outro, route, text, combat or progress changed'}
 $path='src/server/scripts/Scenario/Artifacts/Monk/Intro/chen_presentation.h';$header=Get-Content "$repo/$path" -Raw;$header=Get-ChenPreEscortSource $path $header;$oldHeader=Prior $path
 $normal=$header.Replace('#include "ScriptedCreature.h"','').Replace('uint8 rescueIssued = 0;','').Replace('bool rescueUpdating = false;','')
 foreach($method in @('bool Student(','void UpdateRescue(')){$normal=$normal.Replace((Block $header $method),'')}
 $normal=$normal.Replace(' rescueIssued = 0;','')
 if((Shape $normal) -cne (Shape $oldHeader)){throw 'Existing late presentation ownership, timing or payload changed'}
 foreach($name in @('chen_rescue.h','jorvinax_defeat.h','monk_fel_stone.h')){$path="src/server/scripts/Scenario/Artifacts/Monk/Intro/$name";if((Shape (Get-ChenPreEscortSource $path (Get-Content "$repo/$path" -Raw))) -cne (Shape (Prior $path))){throw 'Original receipt/route/victory/Fel Stone changed'}}
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/instance_class_art_scenarios.cpp';$instances=Get-Content "$repo/$path" -Raw;$oldInstances=Prior $path
 if($hasNine){$instances=(& git -C $repo show "f67c49e3b:$path") -join "`n";if($LASTEXITCODE){throw 'Certified pre-Nine instance missing'}}
 if(([regex]::Matches($instances,[regex]::Escape('chenPresentation.UpdateRescue(diff); '))).Count -ne 1 -or (Shape $instances.Replace('chenPresentation.UpdateRescue(diff); ','')) -cne (Shape $oldInstances)){throw 'Only native early update before existing instance calls required'}
 if($ScopeOnly){'PASS exact original4/5 callbacks/redundant8s wrapper scoped out, two padding flags/two early methods and one before-route update; all late/receipt/route/victory/Fel Stone/other actor+instance bodies retained';return}
 & "$PSScriptRoot/Test-MonkChenPresentation.ps1" -ExportOnly -OutputDirectory $out
 if(!$?){throw 'Current native early host export failed'}
 [IO.File]::WriteAllText("$out/EarlyBaselinePresentation.inc",$oldHeader.Replace('#include "chen_rescue.h"','#include "Rescue.inc"').Replace('namespace MonkChenPresentation','namespace MonkChenPresentationEarlyBaseline'))
 [IO.File]::WriteAllText("$out/EarlyBaselineActor.inc",$oldActor.Replace('npc_monk_chen_porter','npc_monk_chen_porter_early_baseline')+';')
 [IO.File]::WriteAllText("$out/EarlyBaselineInstance.inc",(Block $oldInstances 'struct instance_monk_intro :').Replace('instance_monk_intro','instance_monk_intro_early_baseline').Replace('MonkChenPresentation::State','MonkChenPresentationEarlyBaseline::State')+';')
 if($ExportOnly){return}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/MonkChenEarlyPresentationTest.cpp`" /Fe`"$out/early.exe`" /Fo`"$out/early.obj`" /Fd`"$out/early.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/early.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Actual Chen early presentation ASAN failed'}
}
