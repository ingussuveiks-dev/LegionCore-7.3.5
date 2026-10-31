param([switch]$VerifyReleaseBinary,[string]$DataDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$DataDirectory){$DataDirectory="$repo/build-extractors/runtime-test/sharasdal-upper-voice-data"};if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-upper-voice"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Test-SharasdalFinaleVoice.ps1" -PrepareHost -OutputDirectory $out
if(!$?){throw 'Native conversation host preparation failed'}
$source=Get-Content "$PSScriptRoot/SharasdalFinaleVoiceTest.cpp" -Raw;$end=$source.IndexOf('static void Load(');if($end -lt 0){throw 'Native reusable conversation host boundary changed'}
[IO.File]::WriteAllText("$out/VoiceHost.inc",$source.Substring(0,$end))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/SharasdalUpperVoiceTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$DataDirectory/baseline-conversation.tsv`" `"$DataDirectory/conversation.tsv`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native upper conversation payload/budget regression failed'}
if($VerifyReleaseBinary){$prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$prior";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_hruthnir_conversation_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical native conversation payload/expiry implementation differs'}}finally{$env:PYTHONPATH=$prior}}
