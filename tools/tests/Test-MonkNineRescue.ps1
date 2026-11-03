param([switch]$ScopeOnly,[switch]$ExportOnly,[switch]$VerifyNativeCatalog,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
. "$PSScriptRoot/HaitOpeningPredecessor.ps1"
. "$PSScriptRoot/ChenEscortSpeechPredecessor.ps1"
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/monk-nine-rescue"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence/$(Get-Date -Format yyyyMMdd-HHmmss-fff)" -RuntimeDirectories @($repo,$out) -Test {
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
 function Shape([string]$s){(($s -replace '(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
 function Prior([string]$path){$s=(& git -C $repo show "cfe6cd302:$path") -join "`n";if($LASTEXITCODE){throw "Missing pinned $path"};$s}
 $path='src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_intro_scenario.cpp';$source=Get-Content "$repo/$path" -Raw;$source=Get-HaitPreOpeningSource $path $source;$prior=Prior $path
 if($source.Contains('if (!MonkChenRescue::Source(me)) _presentationFunctions.AddDelayedEvent(4443,')){& "$PSScriptRoot/Test-ChenOpeningVoice.ps1" -ScopeOnly;$source=$source.Replace('if (!MonkChenRescue::Source(me)) _presentationFunctions.AddDelayedEvent(4443,','if (!MonkChenRescue::Source(me)) _presentationFunctions.AddDelayedEvent(4000,')}
 $actor=Block $source 'class npc_nine_czi :';$oldActor=Block $prior 'class npc_nine_czi :'
 $normal=$actor.Replace('if (MonkNineRescue::Source(me)) return;','')
 $normal=$normal.Replace((Block $normal 'if (MonkNineRescue::Source(me))'),'')
 if((Shape $normal) -cne (Shape $oldActor) -or (Shape $source.Replace($actor,$oldActor).Replace('#include "nine_czi_rescue.h"','')) -cne (Shape $prior)){throw 'Only three scoped initial Nine bridges may change; full late/unscoped and other actors retained'}
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/instance_class_art_scenarios.cpp';$instances=Get-Content "$repo/$path" -Raw;$instances=Get-ChenPreEscortSource $path $instances;$oldInstances=Prior $path
 $normal=$instances.Replace('#include "../../Monk/Intro/nine_czi_rescue.h"','').Replace(', nineRescue(new MonkNineRescue::State(&chenRescue))','').Replace('std::unique_ptr<MonkNineRescue::State> nineRescue;','').Replace(' nineRescue->Observe(actor);','').Replace('nineRescue->Death(actor, killer); ','').Replace('if (type == MonkNineRescue::RequestData) nineRescue->Request(value);','').Replace('nineRescue->Update(diff); ','')
 if((Shape $normal) -cne (Shape $oldInstances)){throw 'Other instance/controller behavior changed'}
 foreach($name in @('chen_rescue.h','chen_arrival.h','chen_routes.h','chen_presentation.h','jorvinax_defeat.h','monk_fel_stone.h','nine_czi_arrival.h')){$path="src/server/scripts/Scenario/Artifacts/Monk/Intro/$name";$currentHeader=Get-ChenPreEscortSource $path (Get-Content "$repo/$path" -Raw);if($name -eq 'chen_presentation.h' -and $currentHeader.Contains('elapsed >= 4443')){& "$PSScriptRoot/Test-ChenOpeningVoice.ps1" -ScopeOnly;$currentHeader=$currentHeader.Replace('elapsed >= 4443','elapsed >= 4000')};if((Shape $currentHeader) -cne (Shape (Prior $path))){throw "Prior controller changed: $name"}}
 if($ScopeOnly){'PASS exact initial362181 LOS/death/movement bridges, appended owned state and original instance endpoints; other actors/controllers/native/unscoped payload retained';return}
 & "$PSScriptRoot/Test-MonkNineArrival.ps1" -ExportOnly -OutputDirectory $out
 if(!$?){throw 'Native retained Nine/Chen export failed'}
 [IO.File]::WriteAllText("$out/InitialBaselineActor.inc",$oldActor.Replace('npc_nine_czi','npc_nine_czi_rescue_baseline')+';')
 $movement=Get-Content "$repo/src/server/game/Movement/MovementGenerators/PointMovementGenerator.cpp" -Raw
 [IO.File]::WriteAllText("$out/PointFinalize.inc",(Block $movement 'void PointMovementGenerator<T>::DoFinalize(').Insert(0,"template<class T>`n")+(Block $movement 'void PointMovementGenerator<Creature>::MovementInform(').Insert(0,"template<>`n"))
 [IO.File]::WriteAllText("$out/MovementMedium.inc",(Block (Get-Content "$repo/src/server/game/Movement/MovementGenerator.h" -Raw) 'template<class T, class D>')+';')
 if($ExportOnly){return}
 if($VerifyNativeCatalog){& "$PSScriptRoot/Test-MonkNineArrival.ps1" -VerifyNativeCatalog -OutputDirectory "$out/retained-native";if(!$?){throw 'Matching full943/taxi native retained suite failed'}}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/MonkNineRescueTest.cpp`" /Fe`"$out/rescue.exe`" /Fo`"$out/rescue.obj`" /Fd`"$out/rescue.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/rescue.exe`"`nexit /b %errorlevel%`n")
 $info=New-Object Diagnostics.ProcessStartInfo
 $info.FileName=$env:ComSpec;$info.Arguments='/d /c ""'+$out+'/run.cmd""';$info.WorkingDirectory=$out
 $info.UseShellExecute=$false;$info.CreateNoWindow=$true;$info.RedirectStandardOutput=$true;$info.RedirectStandardError=$true
 $owned=New-Object Diagnostics.Process;$owned.StartInfo=$info
 if(!$owned.Start()){throw 'Owned native fixture launch failed'}
 $ownedPid=$owned.Id;$stdout=$owned.StandardOutput.ReadToEndAsync();$stderr=$owned.StandardError.ReadToEndAsync()
 if(!$owned.WaitForExit(60000)){throw "Owned fixture $ownedPid requires investigation; no user process stopped"}
 $exit=$owned.ExitCode
 [IO.File]::WriteAllText("$out/driver.stdout.log",$stdout.Result);[IO.File]::WriteAllText("$out/driver.stderr.log",$stderr.Result)
 [IO.File]::WriteAllText("$out/driver-result.json",(@{PID=$ownedPid;Exit=$exit}|ConvertTo-Json))
 $owned.Dispose()
 Get-Content "$out/driver.stdout.log"
 Get-Content "$out/driver.stderr.log"
 if($exit){throw "Actual initial Nine ASAN failed exit$exit"}
 if($stderr.Result.Length){throw 'Unexpected native stderr'}
 if($VerifyReleaseBinary){
  if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
  $prior=$env:PYTHONPATH
  try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$prior";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_monk_nine_rescue_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Actual initial Nine Release implementation missing'}}finally{$env:PYTHONPATH=$prior}
 }
}
