param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/devourer-combat-voice/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp" -Raw
 . "$PSScriptRoot/DevourerOpeningCertificate.ps1"
 $source=Restore-DevourerOpeningBoundary $source
 . "$PSScriptRoot/DevourerCombatCertificate.ps1"
 [void](Restore-DevourerCombatBoundary $source)
 $pinned=(& git -C $repo show '48ec2ea80:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned combat source missing'}
 $ai=Block $source 'struct npc_akaari_shadowgore_lastAI : ScriptedAI';$prior=Block $pinned 'struct npc_akaari_shadowgore_lastAI : ScriptedAI'
 $start=$ai.IndexOf('        uint32 combatVoiceDelay');$last=Block $ai 'void RequestCombatVoice('; $end=$ai.IndexOf($last,$start)+$last.Length
 [IO.File]::WriteAllText("$out/CombatVoice.inc",$ai.Substring($start,$end-$start))
 foreach($name in @('void Reset(','void EnterCombat(','void UpdateAI(')){
  $label=if($name -like '*Reset*'){'Reset'}elseif($name -like '*EnterCombat*'){'Enter'}else{'Update'}
  [IO.File]::WriteAllText("$out/Current$label.inc",(Block $ai $name));[IO.File]::WriteAllText("$out/Before$label.inc",(Block $prior $name))
 }
 $eh=Get-Content "$repo/src/server/game/AI/EventMap.h" -Raw;$es=Get-Content "$repo/src/server/game/AI/EventMap.cpp" -Raw
 [IO.File]::WriteAllText("$out/EventMapClass.inc",(Block $eh 'enum c_events')+';'+(Block $eh 'class TC_GAME_API EventMap').Replace('TC_GAME_API ','')+';')
 [IO.File]::WriteAllText("$out/EventMapMethods.inc",((@('void EventMap::Reset(','void EventMap::ScheduleEvent(uint32 eventId, uint32 time','uint32 EventMap::ExecuteEvent(','void EventMap::CancelEvent(')|ForEach-Object {Block $es $_}) -join "`n"))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/DevourerCombatVoiceTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b %errorlevel%`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" > "$out/stdout.log" 2> "$out/stderr.log";$code=$LASTEXITCODE;Get-Content "$out/stdout.log";if($code -or (Get-Item "$out/stderr.log").Length){Get-Content "$out/stderr.log" -Tail 20;throw "Combat voice ASAN exit=$code or stderr"}
}
