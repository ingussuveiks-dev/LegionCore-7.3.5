param([switch]$ScopeOnly,[switch]$ExportOnly,[switch]$VerifyNativeCatalog,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/monk-nine-arrival"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence/$(Get-Date -Format yyyyMMdd-HHmmss-fff)" -RuntimeDirectories @($repo,$out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
 function Shape([string]$s){(($s -replace '(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
 function PriorInitial([string]$path){$s=(& git -C $repo show "cfe6cd302:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-initial missing'};$s}
 function Prior([string]$path){$s=(& git -C $repo show "f67c49e3b:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Nine arrival predecessor missing'};$s}
 $path='src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_intro_scenario.cpp';$source=Get-Content "$repo/$path" -Raw;$old=Prior $path
 $currentActor=Block $source 'class npc_nine_czi :'
 if($source.Contains('#include "nine_czi_rescue.h"')){& "$PSScriptRoot/Test-MonkNineRescue.ps1" -ScopeOnly;$source=PriorInitial $path}
 $actor=Block $source 'class npc_nine_czi :';$oldActor=Block $old 'class npc_nine_czi :';$bridge=Block $actor 'if (MonkNineArrival::Source(me))'
 if((Shape $bridge) -cne (Shape 'if (MonkNineArrival::Source(me)) { if (InstanceScript* script = me->GetInstanceScript()) script->SetGuidData(MonkNineArrival::RequestData, who->GetGUID()); return; }')){throw 'Only the actual LOS visitor may request a scoped arrival'}
 if((Shape $actor.Replace($bridge,'')) -cne (Shape $oldActor) -or (Shape $source.Replace($actor,$oldActor).Replace('#include "nine_czi_arrival.h"','')) -cne (Shape $old)){throw 'Initial Nine/unscoped/other actor/native motion/text changed'}
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/instance_class_art_scenarios.cpp';$instances=Get-Content "$repo/$path" -Raw;$oldInstances=Prior $path
 if($instances.Contains('MonkNineRescue::State')){$instances=PriorInitial $path}
 $normal=$instances.Replace('#include "../../Monk/Intro/nine_czi_arrival.h"','').Replace(', nineArrival(&chenRescue)','').Replace('MonkNineArrival::State nineArrival;','').Replace(' nineArrival.Observe(actor);','').Replace('if (type == MonkNineArrival::RequestData) nineArrival.Request(value);','').Replace('nineArrival.Update(diff); ','')
 if((Shape $normal) -cne (Shape $oldInstances)){throw 'Other instance/base/Chen/Jorvinax behavior changed'}
 foreach($name in @('chen_rescue.h','chen_arrival.h','chen_routes.h','chen_presentation.h','jorvinax_defeat.h','monk_fel_stone.h')){$path="src/server/scripts/Scenario/Artifacts/Monk/Intro/$name";$currentHeader=Get-Content "$repo/$path" -Raw;if($name -eq 'chen_presentation.h' -and $currentHeader.Contains('elapsed >= 4443')){& "$PSScriptRoot/Test-ChenOpeningVoice.ps1" -ScopeOnly;$currentHeader=$currentHeader.Replace('elapsed >= 4443','elapsed >= 4000')};if((Shape $currentHeader) -cne (Shape (Prior $path))){throw 'Prior receipt/presentation/route/victory/Fel Stone changed'}}
 if($ScopeOnly){'PASS exact late362252 actual LOS visitor bridge and appended instance state; entire initial Nine/unscoped/Chen/Hait/native taxi and all previous controllers/instances retained';return}
 & "$PSScriptRoot/Test-MonkChenEarlyPresentation.ps1" -ExportOnly -OutputDirectory $out
 if(!$?){throw 'Actual native Nine arrival host export failed'}
 [IO.File]::WriteAllText("$out/NineActor.inc",$currentActor+';')
 [IO.File]::WriteAllText("$out/NineBaselineActor.inc",$oldActor.Replace('npc_nine_czi','npc_nine_czi_arrival_baseline')+';')
 [IO.File]::WriteAllText("$out/NineBaselineInstance.inc",(Block $oldInstances 'struct instance_monk_intro :').Replace('instance_monk_intro','instance_monk_intro_nine_baseline')+';')
 if($ExportOnly){return}
 if($VerifyNativeCatalog){
  $prior=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile MonkNineArrivalCatalog.cpp -ProbeName MonkNineArrivalCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Fresh native Nine arrival catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
  $native=Get-Content "$out/native.tsv"
  foreach($expected in @("TAXI_PATH`t5333`t1748`t1748`t0","EFFECT`t193724`t0`t0`t123`t0`t5333`t0`t0`t1`t0`t0","EFFECT`t193724`t0`t1`t90`t0`t80166`t0`t0`t1`t0`t0","CRITERION`t1897`t29069`t92`t46902`t0`t0","COMPLETE`t16`t7")){if($native -notcontains $expected){throw 'Matching native taxi/effect/arrival criterion changed'}}
  $nodes=@($native|Where-Object {$_ -like "TAXI_NODE`t*"});if($nodes.Count -ne 10){throw 'Native full ten-node5333 path changed'}
  foreach($index in 0..9){$rows=@($nodes|Where-Object {$_.Split("`t")[2] -ceq [string]$index});if($rows.Count -ne 1 -or $rows[0].Split("`t")[3] -cne '1014'){throw 'Native path indexes/maps differ'}}
  if($native -notcontains "TAXI_NODE`t96448`t9`t1014`t3833.37`t1728.04`t946.423`t0`t0`t46902`t0"){throw 'Native arrival event/endpoint changed'}
  'PASS native taxi5333 all ten real nodes, ordinary193724/123, personal80166 and native46902/29069 tree; actual flight/client remain separate'
 }
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/MonkNineArrivalTest.cpp`" /Fe`"$out/arrival.exe`" /Fo`"$out/arrival.obj`" /Fd`"$out/arrival.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/arrival.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Actual native Nine arrival ASAN failed'}
}
