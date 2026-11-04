param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/althea-intro-asan/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp';$current=Get-Content "$repo/$path" -Raw;$old=(& git -C $repo show "9303d0c59:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned intro predecessor missing'}
 $ai=Block $current 'struct npc_sister_althea_ebonlockeAI : ScriptedAI';$prior=Block $old 'struct npc_sister_althea_ebonlockeAI : ScriptedAI'
 $a=$ai.IndexOf('        bool introBusy =');$b=$ai.IndexOf('        void Reset()',$a);$helpers=$ai.Substring($a,$b-$a)
 [IO.File]::WriteAllText("$out/IntroHelpers.inc",$helpers)
 $normalized=$ai.Remove($a,$b-$a)
 foreach($name in @('void MoveInLineOfSight(','void UpdateAI(')){
  $body=Block $ai $name;$filename=if($name -like 'void Move*'){'MoveInLineOfSight.inc'}else{'UpdateAI.inc'}
  [IO.File]::WriteAllText("$out/$filename",$body)
  $normalized=$normalized.Replace($body,(Block $prior $name))
 }
 if(($current.Replace($ai,$normalized).Replace('#include "KingslayersGuardProgress.h"','')-replace '\s','') -cne ($old-replace '\s','')){throw 'Only Althea intro helpers/LOS, dedicated guard declaration and UpdateIntro dispatch may change'}
 [IO.File]::WriteAllText("$out/GuardIds.inc",([IO.File]::ReadAllText("$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/KingslayersGuardProgress.h").Replace('#include "Define.h"','')))
 $update=Block $ai 'void UpdateAI(';$oldUpdate=Block $prior 'void UpdateAI('
 $expected='void UpdateAI(uint32 diff) override { UpdateIntro(diff);'+$oldUpdate.Substring($oldUpdate.IndexOf('if (!UpdateVictim())'))
 if(($update-replace '\s','') -cne ($expected-replace '\s','')){throw 'Original combat UpdateAI tail changed'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/AltheaIntroTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Actual Althea intro ASAN failed'}
}
