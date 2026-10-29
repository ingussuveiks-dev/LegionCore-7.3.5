param()
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-smart-links"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 function Section([string]$text,[string]$start,[string]$end){
  $i=$text.IndexOf($start);$j=$text.IndexOf($end,[Math]::Max(0,$i))
  if($i -lt 0 -or $j -le $i){throw "Missing native section $start"}
  $text.Substring($i,$j-$i)
 }
 $source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 if((Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw) -notmatch 'SMART_EVENT_LINK\s*=\s*61\s*,'){throw 'Native LINK enum changed; update probe/schema mapping'}
 [IO.File]::WriteAllText("$out/FindLinkedEvent.inc",(Section $source 'SmartScriptHolder SmartScript::FindLinkedEvent(' 'void SmartScript::OnUpdate('))
 [IO.File]::WriteAllText("$out/LinkTail.inc",(Section $source '    if (e.link && e.link != e.event_id)' 'void SmartScript::InstallTemplate('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/ArtifactSmartLinksTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" > `"$out/native.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native LINK execution regression failed'}
 & 'C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' "$PSScriptRoot/verify_artifact_smart_links.py" --native "$out/native.tsv"
 if($LASTEXITCODE){throw 'Audit/native LINK differential failed'}
}
