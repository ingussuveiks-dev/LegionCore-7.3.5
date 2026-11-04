param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/kingslayers-door-asan/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp';$current=Get-Content "$repo/$path" -Raw;$old=(& git -C $repo show "a9e86af7b:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned door predecessor missing'}
 . "$PSScriptRoot/AltheaActorIdentityCertificate.ps1"
 $current=Restore-AltheaActorIdentityBoundary $current
 # Garona reveal has its own whole-source/ASAN/native certificate.
 $current=$current.Replace((Block $current 'class npc_garona_halforcen_outro : public CreatureScript'),(Block $old 'class npc_garona_halforcen_outro : public CreatureScript'))
 # Merlis intro has its own whole-source/ASAN/native certificate.
 $current=$current.Replace((Block $current 'class npc_merlis_malagan : public CreatureScript'),(Block $old 'class npc_merlis_malagan : public CreatureScript'))
 $door=Block $current 'class go_kingslayers_door : public GameObjectScript';$prior=Block $old 'class go_kingslayers_door : public GameObjectScript'
 # New Althea intro has its own whole-source/ASAN/native certificate. Restore
 # only those helpers and dispatch before retaining this earlier door boundary.
 $intro=Block $current 'struct npc_sister_althea_ebonlockeAI : ScriptedAI';$priorIntro=Block $old 'struct npc_sister_althea_ebonlockeAI : ScriptedAI'
 $a=$intro.IndexOf('        bool introBusy =');$b=$intro.IndexOf('        void Reset()',$a);if($a -lt 0 -or $b -lt 0){throw 'Actual intro boundary missing'};$normalized=$intro.Remove($a,$b-$a)
 foreach($name in @('void MoveInLineOfSight(','void UpdateAI(')){$normalized=$normalized.Replace((Block $intro $name),(Block $priorIntro $name))}
 $current=$current.Replace($intro,$normalized)
 if(($current.Replace($door,$prior).Replace('#include "ScriptsData.h"','').Replace('#include "KingslayersGuardProgress.h"','')-replace '\s','') -cne ($old-replace '\s','')){throw 'Only isolated door and native ScriptsData declaration may change'}
 [IO.File]::WriteAllText("$out/DoorAI.inc",(Block $door 'struct go_kingslayers_doorAI : public GameObjectAI')+';')
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/KingslayersDoorTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Actual door AI ASAN failed'}
}
