param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/event-processor-abort/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing native $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $header=Get-Content "$repo/src/common/Utilities/EventProcessor.h" -Raw
 $source=Get-Content "$repo/src/common/Utilities/EventProcessor.cpp" -Raw
 if($Baseline){$source=(& git -C $repo show 'b9ea7fcb5:src/common/Utilities/EventProcessor.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned EventProcessor unavailable'}}
 [IO.File]::WriteAllText("$out/EventProcessor.inc",(Block $header 'class TC_COMMON_API BasicEvent').Replace('TC_COMMON_API ','')+';typedef std::multimap<uint64, BasicEvent*> EventList;'+(Block $header 'class TC_COMMON_API EventProcessor').Replace('TC_COMMON_API ','')+';')
 [IO.File]::WriteAllText("$out/EventMethods.inc",$source.Substring($source.IndexOf('BasicEvent::BasicEvent()')))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $def=if($Baseline){'/DEVENT_ABORT_BASELINE'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $def /I`"$out`" `"$PSScriptRoot/EventProcessorAbortTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b %errorlevel%`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" > "$out/stdout.log" 2> "$out/stderr.log";$code=$LASTEXITCODE;Get-Content "$out/stdout.log"
 if($code -or (Get-Item "$out/stderr.log").Length){Get-Content "$out/stderr.log" -Tail 12;throw "EventProcessor abort ASAN exit=$code/stderr"}
}
