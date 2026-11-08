param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/althea-guards-asan/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/instance_kingslayers.cpp';$current=Get-Content "$repo/$path" -Raw;$old=(& git -C $repo show "b1fc9b11d:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned guard controller predecessor missing'}
 . "$PSScriptRoot/KingslayersGraveyardScope.ps1";$current=Restore-KingslayersGraveyardBoundary $current
 $a=$current.IndexOf('        struct GuardState');$b=$current.IndexOf('        WorldLocation loc_res_pla;',$a);$helpers=$current.Substring($a,$b-$a)
 [IO.File]::WriteAllText("$out/GuardLedger.inc",$helpers)
 if(($current.Remove($a,$b-$a).Replace('#include "KingslayersGuardProgress.h"','').Replace('#include "Map.h"','')-replace '\s','') -cne ($old-replace '\s','')){throw 'Original instance graveyard/update/constructor changed outside dedicated guard ledger'}
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp';$current=Get-Content "$repo/$path" -Raw;$old=(& git -C $repo show "b1fc9b11d:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned guard consumer predecessor missing'}
 . "$PSScriptRoot/AltheaActorIdentityCertificate.ps1"
 $current=Restore-AltheaActorIdentityBoundary $current
 # Merlis intro has its own whole-source/ASAN/native certificate.
 $current=$current.Replace((Block $current 'class npc_merlis_malagan : public CreatureScript'),(Block $old 'class npc_merlis_malagan : public CreatureScript'))
 # The later reveal is independently certified by its whole-source suite.
 $current=$current.Replace((Block $current 'class npc_garona_halforcen_outro : public CreatureScript'),(Block $old 'class npc_garona_halforcen_outro : public CreatureScript'))
 $ai=Block $current 'struct npc_sister_althea_ebonlockeAI : ScriptedAI';$before=Block $old 'struct npc_sister_althea_ebonlockeAI : ScriptedAI';$normalized=$ai
 foreach($name in @('void UpdateIntro(','void MoveInLineOfSight(')){$normalized=$normalized.Replace((Block $ai $name),(Block $before $name))}
 if(($current.Replace($ai,$normalized).Replace('#include "KingslayersGuardProgress.h"','')-replace '\s','') -cne ($old-replace '\s','')){throw 'Only Althea tracked-guard handoff/query and dedicated declaration may change'}
 [IO.File]::WriteAllText("$out/GuardIds.inc",([IO.File]::ReadAllText("$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/KingslayersGuardProgress.h").Replace('#include "Define.h"','')))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/AltheaGuardsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Actual guard instance receipt ASAN failed'}
}
