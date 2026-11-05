param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/devourer-entrance-asan/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp" -Raw
 . "$PSScriptRoot/DevourerDeathVoiceCertificate.ps1"
 $source=Restore-DevourerDeathVoiceBoundary $source
 . "$PSScriptRoot/DevourerEntranceCertificate.ps1"
 $restored=Restore-DevourerEntranceBoundary $source;$old=(& git -C $repo show '698d34495:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp') -join "`n"
 if($LASTEXITCODE -or ($restored-replace '\s','') -cne ($old-replace '\s','')){throw 'Unreviewed change outside original entrance'}
 [IO.File]::WriteAllText("$out/EntranceAI.inc",(Block (Block $source 'class npc_akaari_shadowgore_last :') 'struct npc_akaari_shadowgore_lastAI :')+';')
 [IO.File]::WriteAllText("$out/SceneStatus.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.h" -Raw) 'enum SceneEventStatus')+';')
 $scope=Get-Content "$repo/src/server/game/Entities/Object/QuestObjectScope.h" -Raw
 [IO.File]::WriteAllText("$out/QuestScope.inc",(Block $scope 'class QuestCancellationReceipt')+';'+(Block $scope "`nstruct QuestObjectScope")+';')
 $eh=Get-Content "$repo/src/server/game/AI/EventMap.h" -Raw;$es=Get-Content "$repo/src/server/game/AI/EventMap.cpp" -Raw
 [IO.File]::WriteAllText("$out/EventMapClass.inc",(Block $eh 'enum c_events')+';'+(Block $eh 'class TC_GAME_API EventMap').Replace('TC_GAME_API ','')+';')
 [IO.File]::WriteAllText("$out/EventMapMethods.inc",((@('void EventMap::Reset(','void EventMap::ScheduleEvent(uint32 eventId, uint32 time','uint32 EventMap::ExecuteEvent(','void EventMap::CancelEvent(')|ForEach-Object {Block $es $_}) -join "`n"))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/DevourerEntranceTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b %errorlevel%`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" > "$out/stdout.log" 2> "$out/stderr.log";$code=$LASTEXITCODE;Get-Content "$out/stdout.log";if($code -or (Get-Item "$out/stderr.log").Length){Get-Content "$out/stderr.log" -Tail 12;throw "Devourer entrance ASAN exit=$code or stderr"}
}
