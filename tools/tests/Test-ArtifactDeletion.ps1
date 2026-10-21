$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-deletion"
New-Item -ItemType Directory -Force $out | Out-Null
$text=Get-Content "$repo/src/server/game/Handlers/ItemHandler.cpp" -Raw
$a=$text.IndexOf('void WorldSession::HandleDestroyItemOpcode(');$end=$text.IndexOf('{',$a)+1;$depth=1
while($depth -and $end -lt $text.Length){if($text[$end] -eq '{'){$depth++};if($text[$end] -eq '}'){$depth--};$end++}
if($a -lt 0 -or $depth){throw 'Native destroy handler extraction failed'}
[IO.File]::WriteAllText("$out/Destroy.inc",$text.Substring($a,$end-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/ArtifactDeletionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Artifact destruction handler regression failed'}
& "$PSScriptRoot/../research/Read-LegionLegendaryCatalog.ps1" -SourceFile ArtifactDeletionCatalog.cpp -ProbeName ArtifactDeletionCatalog
