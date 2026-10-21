param([switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/world-socket-diagnostics"
New-Item -ItemType Directory -Force $out | Out-Null
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
# Only this ABI fixture targets Windows 10 so mstcpip.h exposes TCP_INFO_v0.
# Production keeps its existing Windows 7 target and optional runtime IOCTL.
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /MD /D_WIN32_WINNT=0x0A00 /DNTDDI_VERSION=0x0A000003 /I`"C:/local/boost_1_85_0`" /I`"$repo/src/server/game/Server`" `"$PSScriptRoot/WorldSocketDiagnosticsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /link ws2_32.lib mswsock.lib`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native world socket TCP diagnostics regression failed'}
if($VerifyReleaseBinary){
 if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
 $python=(Get-Command python -ErrorAction SilentlyContinue | Select-Object -First 1).Source
 if(!$python){$python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
 & $python "$PSScriptRoot/verify_world_socket_release.py" $ReleaseDirectory
 if($LASTEXITCODE){throw 'Actual Release lacks handshake diagnostic implementation'}
}
