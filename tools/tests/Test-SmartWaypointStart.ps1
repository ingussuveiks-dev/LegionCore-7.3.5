param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/smart-waypoint-start"
New-Item -ItemType Directory -Force $out | Out-Null
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
function Source([string]$path){if($Baseline){$result=(& git -C $repo show "42771a162a19423de14e9c05855c5c963a9f6662:$path") -join "`n";if($LASTEXITCODE){throw 'Cannot read pinned baseline'};return $result};Get-Content "$repo/$path" -Raw}
$source=Source 'src/server/game/AI/SmartScripts/SmartScript.cpp'
$sections=@{
 'RepeatGuard'=@('    if ((e.event.event_phase_mask && !IsInPhase(','    ConditionList conds =')
 'DataEvent'=@('        case SMART_EVENT_DATA_SET:','        case SMART_EVENT_PASSENGER_REMOVED:')
 'ActionStart'=@('    e.runOnce = true;','    if (Unit* tempInvoker = GetLastInvoker())')
 'WaypointAction'=@('        case SMART_ACTION_WP_START:','        case SMART_ACTION_WP_PAUSE:')
 'LinkTail'=@('    if (e.link && e.link != e.event_id)','void SmartScript::InstallTemplate(')
}
foreach($name in $sections.Keys){[IO.File]::WriteAllText("$out/$name.inc",(Section $source $sections[$name][0] $sections[$name][1]))}
$source=Source 'src/server/game/AI/SmartScripts/SmartAI.cpp'
$start=if($Baseline){'void SmartAI::StartPath('}else{'bool SmartAI::StartPath('}
[IO.File]::WriteAllText("$out/StartPath.inc",(Section $source $start 'bool SmartAI::LoadPath('))
[IO.File]::WriteAllText("$out/LoadPath.inc",(Section $source 'bool SmartAI::LoadPath(' 'void SmartAI::PausePath('))
[IO.File]::WriteAllText("$out/GetNextWayPoint.inc",(Section $source 'WayPoint* SmartAI::GetNextWayPoint(' 'bool SmartAI::HasEscortState('))
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $sql='SELECT entryorguid,source_type,id,event_type,event_flags,link,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6 FROM smart_scripts WHERE action_type=53 ORDER BY entryorguid,source_type,id;'
 $rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Read-only waypoint shape census failed'}
}finally{$env:MYSQL_PWD=$previous}
[IO.File]::WriteAllLines("$out/shapes.tsv",[string[]]$rows)
$define=if($Baseline){'/DWAYPOINT_BASELINE'}else{''}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $define /I`"$out`" `"$PSScriptRoot/SmartWaypointStartTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/shapes.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native waypoint-start regression failed'}
if($VerifyReleaseBinary){
 if($Baseline){throw 'Baseline host cannot certify a repaired Release'}
 $previousPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_smart_waypoint_start_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical waypoint PE verification failed'}}finally{$env:PYTHONPATH=$previousPython}
}
