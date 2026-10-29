param([string]$DataDirectory="$PSScriptRoot/../../build-extractors/runtime-test/tak-tak-route-data")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/tak-tak-route"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 function Section([string]$text,[string]$start,[string]$end){$i=$text.IndexOf($start);$j=$text.IndexOf($end,[Math]::Max(0,$i));if($i -lt 0 -or $j -le $i){throw "Missing native $start"};$text.Substring($i,$j-$i)}
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 [IO.File]::WriteAllText("$out/FindLinkedEvent.inc",(Section $s 'SmartScriptHolder SmartScript::FindLinkedEvent(' 'void SmartScript::OnUpdate('))
 [IO.File]::WriteAllText("$out/LinkTail.inc",(Section $s '    if (e.link && e.link != e.event_id)' 'void SmartScript::InstallTemplate('))
 [IO.File]::WriteAllText("$out/GossipDispatch.inc",(Section $s 'void SmartScript::ProcessEventsFor(' 'void SmartScript::ProcessAction('))
 [IO.File]::WriteAllText("$out/GossipEvent.inc",(Section $s '        case SMART_EVENT_GOSSIP_SELECT:' '        case SMART_EVENT_DUMMY_EFFECT:'))
 [IO.File]::WriteAllText("$out/LinkEvent.inc",(Section $s '        case SMART_EVENT_LINK: //special handling' '        //called from Update tick'))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $data=[IO.Path]::GetFullPath($DataDirectory)
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/TakTakRouteTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$data/original.tsv`" `"$data/repaired.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Tak-Tak gossip/LINK regression failed'}
}
