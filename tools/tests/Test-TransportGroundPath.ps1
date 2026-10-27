param([switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/transport-ground-path"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories $out -Test {
 foreach($name in @('Transport.h','GameObjectModel.h','MoveSplineInitArgs.h')){[IO.File]::WriteAllText("$out/$name",'// Mock model boundary; production helper and separate actual ship tests.')}
 $prior=(& git -C $repo show 'c734844c:src/server/game/Movement/TransportGroundPath.h') -join "`n"
 if($LASTEXITCODE){throw 'Pinned Charge ground baseline missing'}
 $prior=$prior.Replace('LEGION_TRANSPORT_GROUND_PATH_H','LEGION_PRIOR_TRANSPORT_GROUND_PATH_H').Replace('namespace Movement','namespace PriorCharge')
 [IO.File]::WriteAllText("$out/PriorTransportGroundPath.inc",$prior)
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $flags=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /W4 $flags /I`"$out`" /I`"$repo/src/server/game/Movement`" `"$PSScriptRoot/TransportGroundPathTest.cpp`" /Fe:test.exe /Fo:test.obj /Fd:test.pdb`nif errorlevel 1 exit /b 1`ntest.exe`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Transport ground regression failed'}
}
