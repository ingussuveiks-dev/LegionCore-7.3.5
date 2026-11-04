param([switch]$Baseline,[ValidateSet('far','near')][string]$Case='far')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/transport-teleport-passengers/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $path='src/server/game/Entities/Transport/Transport.cpp';$source=Get-Content "$repo/$path" -Raw;$old=(& git -C $repo show "fe118183d:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned transport baseline missing'}
 $body=Block $source 'bool Transport::TeleportTransport('; $prior=Block $old 'bool Transport::TeleportTransport('
 if(($source.Replace($body,$prior).Replace('#include <utility>','').Replace('#include <vector>','')-replace '\s','') -cne ($old-replace '\s','')){throw 'Unreviewed change outside transport transfer loop'}
 [IO.File]::WriteAllText("$out/TransportTeleport.inc",$(if($Baseline){$prior}else{$body}))
 [IO.File]::WriteAllText("$out/TransportRemove.inc",(Block $source 'void Transport::RemovePassenger('))
 [IO.File]::WriteAllText("$out/TransportPosition.inc",(Block (Get-Content "$repo/src/server/game/Entities/Vehicle/VehicleDefines.h" -Raw) 'static void CalculatePassengerPosition('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $define=if($Baseline){'/DTRANSPORT_BASELINE'}else{''}
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $define /I`"$out`" `"$PSScriptRoot/TransportTeleportPassengersTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nexit /b %errorlevel%`n")
 & "$out/build.cmd" *> "$out/build.log";if($LASTEXITCODE){Get-Content "$out/build.log" -Tail 16;throw 'Transport transfer ASAN build failed'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`n`"$out/test.exe`" $Case`nexit /b %errorlevel%`n")
 & "$out/run.cmd" > "$out/stdout.log" 2> "$out/stderr.log";$code=$LASTEXITCODE
 [IO.File]::WriteAllText("$out/process-result.json",(@{Exit=$code;Baseline=[bool]$Baseline;Case=$Case}|ConvertTo-Json))
 if($Baseline){
  $failure=Get-Content "$out/stderr.log" -Raw
  if(!$code -or [string]::IsNullOrWhiteSpace($failure) -or $failure -notmatch 'AddressSanitizer: heap-use-after-free'){throw 'Pinned counterfactual did not produce the expected iterator heap-use-after-free'}
  "COUNTERFACTUAL expected-broken original $Case transfer exits $code with captured ASAN heap-use-after-free; not a passing repaired run"
 }else{Get-Content "$out/stdout.log";if($code -or (Get-Item "$out/stderr.log").Length){Get-Content "$out/stderr.log" -Tail 16;throw 'Repaired transport transfer failed'}}
}
