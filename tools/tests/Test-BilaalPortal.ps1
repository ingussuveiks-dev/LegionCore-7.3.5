param([switch]$AddressSanitizer)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/bilaal-portal"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing native block $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){if($s[$b] -eq '{'){++$depth};if($s[$b] -eq '}'){--$depth};++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo,$out) -Test {
 Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/bilaal_portal.h" "$out/Portal.inc"
 $s=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/bilaal_death.h" -Raw
 foreach($name in @('Progress','Victory')){[IO.File]::WriteAllText("$out/$name.inc",(Block $s $(if($name -eq 'Progress'){'Scenario* Progress('}else{'bool Victory('}))) }
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 [IO.File]::WriteAllText("$out/EventLoop.inc",(Block $s 'void SmartScript::ProcessEventsFor('))
 function Slice([string]$name,[string]$start,[string]$end){$a=$s.IndexOf($start);$b=$s.IndexOf($end,$a);if($a -lt 0 -or $b -le $a){throw "Missing $name"};[IO.File]::WriteAllText("$out/$name.inc",$s.Substring($a,$b-$a))}
 Slice CastAction '        case SMART_ACTION_CAST:' '        case SMART_ACTION_CAST_CUSTOM:'
 Slice InstanceAction '        case SMART_ACTION_SET_INST_DATA64:' '        case SMART_ACTION_UPDATE_TEMPLATE:'
 Slice NpcFlagAction '        case SMART_ACTION_SET_NPC_FLAG:' '        case SMART_ACTION_ADD_NPC_FLAG:'
 Slice MoveZAction '        case SMART_ACTION_MOVE_Z:' '        case SMART_ACTION_SET_KD:'
 Slice SelfTarget '        case SMART_TARGET_SELF:' '        case SMART_TARGET_VICTIM:'
 Slice WaypointEvent '        case SMART_EVENT_WAYPOINT_REACHED:' '        case SMART_EVENT_SUMMON_DESPAWNED:'
 $a=$s.IndexOf('void SmartScript::ProcessEvent(');$b=$s.IndexOf('    switch (e.GetEventType())',$a);$a=$s.IndexOf('{',$a)+1
 [IO.File]::WriteAllText("$out/EventPrefix.inc",$s.Substring($a,$b-$a))
 $a=$s.IndexOf('    if (e.link && e.link != e.event_id)');$b=$s.IndexOf('void SmartScript::InstallTemplate',$a)
 [IO.File]::WriteAllText("$out/LinkTail.inc",$s.Substring($a,$b-$a).TrimEnd().TrimEnd('}'))
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
 [IO.File]::WriteAllText("$out/CastFlags.inc",(Block $s 'enum SmartCastFlags')+';')
 $enums='';foreach($name in @('SMART_EVENT','SMART_ACTION','SMARTAI_TARGETS','SmartScriptType','SmartEventFlags')){$enums+=(Block $s "enum $name`r`n")+';'}
 [IO.File]::WriteAllText("$out/NativeEnums.inc",$enums)
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/BilaalPortalTest.cpp`" /Fe:test.exe /Fo:test.obj`nif errorlevel 1 exit /b 1`ntest.exe`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Bilaal portal native receipt/factory regression failed'}
}
