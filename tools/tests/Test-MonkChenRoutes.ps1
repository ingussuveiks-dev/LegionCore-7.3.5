param([switch]$ScopeOnly,[switch]$ExportOnly,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/monk-chen-routes"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence/$(Get-Date -Format yyyyMMdd-HHmmss-fff)" -RuntimeDirectories @($repo,$out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
 function Shape([string]$s){(($s -replace '(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
 function Prior([string]$path){$text=(& git -C $repo show "39f878974:$path") -join "`n";if($LASTEXITCODE){throw "Pinned route predecessor missing: $path"};$text}
 $path='src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_intro_scenario.cpp';$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$old=Prior $path
 $actor=Block $source 'class npc_monk_chen_porter :';$previous=Block $old 'class npc_monk_chen_porter :'
 $normal=$actor.Replace('if (MonkChenRescue::Source(me)) return;','')
 if(([regex]::Matches($actor,[regex]::Escape('if (MonkChenRescue::Source(me)) return;'))).Count -ne 1){throw 'Exactly one scoped15s first-line owner bridge required'}
 foreach($kind in @('Escort','Hait')){
  $marker=$actor.IndexOf("MonkChenRoutes::$($kind)Data");if($marker -lt 0){throw 'Route request missing'}
  $start=$actor.LastIndexOf('if (MonkChenRescue::Source(me))',$marker)
  $guard=Block $actor.Substring($start) 'if (MonkChenRescue::Source(me))'
  $expected="if (MonkChenRescue::Source(me)) { if (InstanceScript* script = me->GetInstanceScript()) script->SetGuidData(MonkChenRoutes::$($kind)Data, me->GetGUID()); return; }"
  if((Shape $guard) -cne (Shape $expected)){throw 'Essential route dispatch bridge changed'}
  $normal=$normal.Replace($guard,'')
 }
 $normal=$normal.Replace('me->GetMotionMaster()->MovePath(9859005, false);',"if (MonkChenRescue::Source(me)) me->UpdateWaypointID(uint32(-1));`nme->GetMotionMaster()->MovePath(9859005, false);`nif (MonkChenRescue::Source(me)) if (InstanceScript* script = me->GetInstanceScript()) script->SetGuidData(MonkChenArrival::RouteData, me->GetGUID());")
 if((Shape $normal) -cne (Shape $previous) -or (Shape $source.Replace($actor,$previous)) -cne (Shape $old)){throw 'Other native Chen/Monk actor behavior changed'}
 $hp='src/server/scripts/Scenario/Artifacts/Monk/Intro/chen_rescue.h';$header=(Get-Content "$repo/$hp" -Raw).Replace("`r",'');$prior=Prior $hp
 $h=$header.Replace('#include "chen_routes.h"','')
 foreach($method in @('void RouteRequest(','void UpdateRoutes(')){$h=$h.Replace((Block $h $method),'')}
 $fields=@'
    ObjectGuid hait, handoffPlayer;
    uint32 escortStartedAt = 0, handoffStartedAt = 0, routeRetryAt = 0;
    bool routeRetrying = false, routeBusy = false, handoffPending = false;
    bool escortSpoken = false, escortFollowed = false, handoffAnnounced = false, handoffSpoken = false;
    bool escortWarned = false, handoffWarned = false;
'@
 $h=$h.Replace($fields.Replace("`r",''),'').Replace('routeRetrying = false; handoffPending = false;','').Replace('if (MonkChenRoutes::Source(actor) && hait.IsEmpty()) hait = actor->GetGUID();','').Replace('if (!pending) { UpdateArrival(diff); UpdateRoutes(); return; }','if (!pending) { UpdateArrival(diff); return; }').Replace('escortStartedAt = getMSTime();','').Replace('handoffStartedAt = getMSTime(); handoffPlayer = member.player; handoffPending = true;','')
 if((Shape $h) -cne (Shape $prior)){throw 'Other original genuine rescue/arrival receipt logic changed'}
 $ip='src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/instance_class_art_scenarios.cpp';$actual=(Get-Content "$repo/$ip" -Raw).Replace("`r",'');$priorInstance=Prior $ip
 $normalInstance=$actual.Replace('if (type == MonkChenRoutes::EscortData) chenRescue.RouteRequest(value);','').Replace('if (type == MonkChenRoutes::HaitData) chenRescue.RouteRequest(value);','')
 if((Shape $normalInstance) -cne (Shape $priorInstance)){throw 'Other Monk or shared instance behavior changed'}
 if($ScopeOnly){'PASS exact two original Chen8s/Hait15s route request bridges and instance-owned receipt clocks; all nine authored deadlines, other payloads/actors/native progression retained';return}
 & "$PSScriptRoot/Test-MonkChenArrival.ps1" -ExportOnly -OutputDirectory $out
 if(!$?){throw 'Current scoped route host export failed'}
 [IO.File]::WriteAllText("$out/RoutesBaselineActor.inc",$previous.Replace('npc_monk_chen_porter','npc_monk_chen_porter_routes_baseline')+';')
 [IO.File]::WriteAllText("$out/RoutesBaselineState.inc",$prior.Replace('namespace MonkChenRescue','namespace MonkChenRescueRoutesBaseline'))
 [IO.File]::WriteAllText("$out/RoutesBaselineInstance.inc",(Block $priorInstance 'struct instance_monk_intro :').Replace('instance_monk_intro','instance_monk_intro_routes_baseline').Replace('MonkChenRescue::State','MonkChenRescueRoutesBaseline::State')+';')
 if($ExportOnly){return}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run-routes.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/MonkChenRoutesTest.cpp`" /Fe`"$out/routes.exe`" /Fo`"$out/routes.obj`" /Fd`"$out/routes.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/routes.exe`"`nexit /b %errorlevel%`n")
 & "$out/run-routes.cmd";if($LASTEXITCODE){throw 'Actual essential routes ASAN regression failed'}
}
