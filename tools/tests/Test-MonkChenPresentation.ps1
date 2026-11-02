param([switch]$ScopeOnly,[switch]$ExportOnly,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/monk-chen-presentation"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence/$(Get-Date -Format yyyyMMdd-HHmmss-fff)" -RuntimeDirectories @($repo,$out) -Test {
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
 function Shape([string]$s){(($s -replace '(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
 function Prior([string]$path){$s=(& git -C $repo show "6cb25d055:$path") -join "`n";if($LASTEXITCODE){throw "Pinned presentation predecessor missing: $path"};$s}
 $path='src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_intro_scenario.cpp';$source=Get-Content "$repo/$path" -Raw;$prior=Prior $path
 $actor=Block $source 'class npc_monk_chen_porter :';$oldActor=Block $prior 'class npc_monk_chen_porter :';$normal=$actor
 foreach($deadline in @(3000,7000,11000,15000,16000)){
  $prefix="if (!MonkChenRescue::Source(me)) _presentationFunctions.AddDelayedEvent($deadline,"
  $count=if($deadline -eq 15000){2}else{1}
  if(([regex]::Matches($actor,[regex]::Escape($prefix))).Count -ne $count){throw 'Four scoped presentation callbacks and two redundant15s wrappers required'}
  $normal=$normal.Replace($prefix,"_presentationFunctions.AddDelayedEvent($deadline,")
 }
 if((Shape $normal) -cne (Shape $oldActor) -or (Shape $source.Replace($actor,$oldActor)) -cne (Shape $prior)){throw 'Other original Chen/Monk actors, timers, text, routes or progression changed'}
 foreach($header in @('chen_rescue.h','jorvinax_defeat.h','monk_fel_stone.h')){$path="src/server/scripts/Scenario/Artifacts/Monk/Intro/$header";if((Shape (Get-Content "$repo/$path" -Raw)) -cne (Shape (Prior $path))){throw 'Original context/victory/Fel Stone changed'}}
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/instance_class_art_scenarios.cpp';$instances=Get-Content "$repo/$path" -Raw;$oldInstances=Prior $path
 $normal=$instances.Replace('#include "../../Monk/Intro/chen_presentation.h"','').Replace(', chenPresentation(&chenRescue)','').Replace('MonkChenPresentation::State chenPresentation;','').Replace('chenPresentation.Update(diff); ','').Replace(' chenPresentation.Update(diff, true);','')
 if((Shape $normal) -cne (Shape $oldInstances)){throw 'Other original/shared instance behavior changed'}
 if($ScopeOnly){'PASS four original3/7/11/16s presentation callbacks/two redundant15s wrappers scoped out, appended instance state before original route update; original context, Jorvinax, Fel Stone, all other actors/instances/credits/routes unchanged';return}
 & "$PSScriptRoot/Test-MonkChenRoutes.ps1" -ExportOnly -OutputDirectory $out
 if(!$?){throw 'Current native host export failed'}
 [IO.File]::WriteAllText("$out/PresentationBaselineActor.inc",$oldActor.Replace('npc_monk_chen_porter','npc_monk_chen_porter_presentation_baseline')+';')
 [IO.File]::WriteAllText("$out/PresentationBaselineInstance.inc",(Block $oldInstances 'struct instance_monk_intro :').Replace('instance_monk_intro','instance_monk_intro_presentation_baseline')+';')
 if($ExportOnly){return}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/MonkChenPresentationTest.cpp`" /Fe`"$out/presentation.exe`" /Fo`"$out/presentation.obj`" /Fd`"$out/presentation.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/presentation.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Actual Chen presentation ASAN failed'}
}
