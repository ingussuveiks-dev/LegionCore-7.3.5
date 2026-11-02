param([switch]$ScopeOnly,[switch]$ExportOnly,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/monk-chen-arrival"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence/$(Get-Date -Format yyyyMMdd-HHmmss-fff)" -RuntimeDirectories @($repo,$out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
 function Shape([string]$s){(($s -replace '(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
 $path='src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_intro_scenario.cpp';$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$old=(& git -C $repo show "b015d4e28:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-arrival actor missing'}
 $hasRoutes=$source.Contains('MonkChenRoutes::EscortData')
 if($hasRoutes){& "$PSScriptRoot/Test-MonkChenRoutes.ps1" -ScopeOnly;$source=(& git -C $repo show "39f878974:$path") -join "`n";if($LASTEXITCODE){throw 'Certified route actor missing'}}
 $actor=Block $source 'class npc_monk_chen_porter :';$previous=Block $old 'class npc_monk_chen_porter :';$movement=Block $actor 'void MovementInform('
 $guard=Block $movement 'if (MonkChenRescue::Source(me))'
 $expectedGuard=@'
if (MonkChenRescue::Source(me))
                    {
                        if (_arrivalStarted) return;
                        InstanceScript* script = me->GetInstanceScript();
                        if (!script || script->GetGuidData(MonkChenArrival::ClaimData) != me->GetGUID())
                        {
                            if (script) script->SetGuidData(MonkChenArrival::RequestData, me->GetGUID());
                            return;
                        }
                        _arrivalStarted = true;
                    }
'@
 if((Shape $guard) -cne (Shape $expectedGuard)){throw 'Native arrival claim/consume bridge changed'}
 $normalMovement=$movement.Replace($guard,'').Replace('if (!MonkChenRescue::Source(me))','')
 $normalMovement=$normalMovement.Replace("ObjectGuid const playerGuid = ArrivalPlayer();`n                    if (!playerGuid.IsEmpty())`n                    {","if (Player* pl = me->FindNearestPlayer(100.0f))`n                    {`n                        ObjectGuid const playerGuid = pl->GetGUID();")
 $normal=$actor.Replace('bool _arrivalStarted = false;','').Replace((Block $actor 'uint32 GetData('),(Block $previous 'uint32 GetData(')).Replace((Block $actor 'ObjectGuid ArrivalPlayer('),'').Replace($movement,$normalMovement)
 $normal=$normal.Replace('if (MonkChenRescue::Source(me)) me->UpdateWaypointID(uint32(-1));','').Replace("if (MonkChenRescue::Source(me))`n                        if (InstanceScript* script = me->GetInstanceScript())`n                            script->SetGuidData(MonkChenArrival::RouteData, me->GetGUID());",'')
 if((Shape $normal) -cne (Shape $previous)){throw 'Other rescue/timer/text/follow/combat/actor behavior changed'}
 if((Shape $source.Replace($actor,$previous)) -cne (Shape $old)){throw 'Other Monk actors changed'}
 $hp='src/server/scripts/Scenario/Artifacts/Monk/Intro/chen_rescue.h';$header=(Get-Content "$repo/$hp" -Raw).Replace("`r",'');$prior=(& git -C $repo show "b015d4e28:$hp") -join "`n";if($LASTEXITCODE){throw 'Pinned rescue controller missing'}
 if($hasRoutes){$header=(& git -C $repo show "39f878974:$hp") -join "`n";if($LASTEXITCODE){throw 'Certified route controller missing'}}
 $h=$header.Replace('#include "chen_arrival.h"','')
 foreach($method in @('bool EscortLeaf()','void Routed(','void Arrived(','void UpdateArrival(')){$h=$h.Replace((Block $h $method),'')}
 $fields=@'
    ObjectGuid arrivalClaim, arrivalPlayer;
    bool routeIssued = false, arrivalSeen = false, arrivalDone = false, arrivalWarned = false;
    uint32 arrivalRetryStart = 0;
    bool arrivalWaiting = false;
'@
 $h=$h.Replace($fields,'').Replace('arrivalClaim.Clear(); arrivalPlayer.Clear(); arrivalDone = started;','').Replace('arrivalWaiting = false;','').Replace('if (!pending) { UpdateArrival(diff); return; }','if (!pending) return;').Replace('{ pending = false; started = true; claim.Clear(); return; }','{ pending = false; started = true; claim.Clear(); members.clear(); return; }')
 if((Shape $h) -cne (Shape $prior)){throw 'Other native genuine-death rescue controller behavior changed'}
 $ip='src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/instance_class_art_scenarios.cpp';$actual=(Get-Content "$repo/$ip" -Raw).Replace("`r",'');$previousInstanceFile=(& git -C $repo show "b015d4e28:$ip") -join "`n";if($LASTEXITCODE){throw 'Pinned Monk instance missing'}
 if($hasRoutes){$actual=(& git -C $repo show "39f878974:$ip") -join "`n";if($LASTEXITCODE){throw 'Certified route instance missing'}}
 $instance=Block $actual 'struct instance_monk_intro :';$oldInstance=Block $previousInstanceFile 'struct instance_monk_intro :'
 $normalInstance=$instance.Replace((Block $instance 'ObjectGuid GetGuidData('),(Block $oldInstance 'ObjectGuid GetGuidData(')).Replace('if (type == MonkChenArrival::RouteData) chenRescue.Routed(value);','').Replace('if (type == MonkChenArrival::RequestData) chenRescue.Arrived(value);','')
 if((Shape $normalInstance) -cne (Shape $oldInstance) -or (Shape $actual.Replace($instance,$oldInstance)) -cne (Shape $previousInstanceFile)){throw 'Other instance/base/shared Rogue/Mage/Priest behavior changed'}
 if($ScopeOnly){'PASS exact arrival-only actor/controller/route/owner/GUID bridges; original rescue, six authored outro deadlines/other three callbacks and all other actors/shared instances retained';return}
 & "$PSScriptRoot/Test-MonkChenRescue.ps1" -ExportOnly -OutputDirectory $out
 if(!$?){throw 'Current native host export failed'}
 [IO.File]::WriteAllText("$out/ArrivalBaselineActor.inc",$previous.Replace('npc_monk_chen_porter','npc_monk_chen_porter_arrival_baseline')+';')
 if($ExportOnly){return}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run-arrival.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/MonkChenArrivalTest.cpp`" /Fe`"$out/arrival.exe`" /Fo`"$out/arrival.obj`" /Fd`"$out/arrival.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/arrival.exe`"`nexit /b %errorlevel%`n")
 & "$out/run-arrival.cmd";if($LASTEXITCODE){throw 'Actual native arrival ASAN regression failed'}
}
