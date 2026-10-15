param([switch]$VerifyReleaseBinary,[string]$BaselineRevision)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/smart-gameobject-flags"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
if($BaselineRevision){$source=(& git -C $repo show "${BaselineRevision}:src/server/game/AI/SmartScripts/SmartScript.cpp") -join "`n";if($LASTEXITCODE){throw 'Cannot read baseline SmartScript'}}
$a=$source.IndexOf('        case SMART_ACTION_SET_GO_FLAG:');$b=$source.IndexOf('        case SMART_ACTION_SET_UNIT_FLAG:',[Math]::Max(0,$a))
if($a -lt 0){$handlers=''}else{if($b -lt $a){throw 'GO flag handler boundary missing'};$handlers=$source.Substring($a,$b-$a)}
[IO.File]::WriteAllText("$out/FlagActions.inc",$handlers)
$a=$source.IndexOf('    if (e.link && e.link != e.event_id)');$b=$source.IndexOf('void SmartScript::InstallTemplate(',$a)
if($a -lt 0 -or $b -lt $a){throw 'Native linked-action dispatch not found'}
[IO.File]::WriteAllText("$out/LinkTail.inc",$source.Substring($a,$b-$a))
$header=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
$constants=@('SMART_ACTION_SET_GO_FLAG','SMART_ACTION_ADD_GO_FLAG','SMART_ACTION_REMOVE_GO_FLAG','SMART_EVENT_LINK') | ForEach-Object {
    $m=[regex]::Match($header,"$_\s*=\s*(\d+)");if(!$m.Success){throw "Missing native enum $_"};"constexpr uint32 $_=$($m.Groups[1].Value);"
}
[IO.File]::WriteAllLines("$out/Constants.inc",[string[]]$constants)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/SmartGameObjectFlagsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" 2>&1`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'SmartAI gameobject flag regression failed'}
if($VerifyReleaseBinary){
    $binary=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe"))
    if(!$binary.Contains('SmartScript::ProcessAction: gameobject flags action')){throw 'Canonical Release lacks GO flag dispatcher'}
    'PASS: canonical Release includes SmartAI gameobject flag actions'
}
