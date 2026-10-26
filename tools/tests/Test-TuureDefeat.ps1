param([switch]$Baseline,[switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/tuure-defeat"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/$(if($Baseline){'baseline'}else{'current'})" -RuntimeDirectories @($out,$repo) -Test {
 $path='src/server/scripts/Scenario/Artifacts/Priest/Tuure/Tuure.cpp';$current=Get-Content "$repo/$path" -Raw;$source=$current
 if($Baseline){$source=(& git -C $repo show "ce34ea6c:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Tuure defeat baseline unavailable'}}
 [IO.File]::WriteAllText("$out/Defeat.inc",(Block $current 'namespace TuureDefeat')+(Block $source 'struct npc_lady_kalindriaAI :')+';')
 $u=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
 [IO.File]::WriteAllText("$out/OwnerGuid.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'ObjectGuid GetCharmerOrOwnerGUID() const'))
 [IO.File]::WriteAllText("$out/OwnerPlayer.inc",(Block $u 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'))
 $a=$u.IndexOf('    damage /= victim->GetHealthMultiplierForTarget(this);');$b=$u.IndexOf('    // A player charmed by an NPC',$a)
 if($a -lt 0 -or $b -le $a){throw 'Native post-scaling dispatch missing'}
 [IO.File]::WriteAllText("$out/ScaledHook.inc",$u.Substring($a,$b-$a))
 $eh=Get-Content "$repo/src/server/game/AI/EventMap.h" -Raw;$es=Get-Content "$repo/src/server/game/AI/EventMap.cpp" -Raw
 [IO.File]::WriteAllText("$out/EventMap.inc",(Block $eh 'enum c_events')+';'+(Block $eh 'class TC_GAME_API EventMap').Replace('TC_GAME_API ','')+';'+((@('void EventMap::Reset(','void EventMap::ScheduleEvent(uint32 eventId, uint32 time','uint32 EventMap::ExecuteEvent(','void EventMap::CancelEvent(')|ForEach-Object {Block $es $_}) -join "`n"))
 $fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
 [IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $def=if($Baseline){'/DBASELINE'}else{''};if($AddressSanitizer){$def+=' /fsanitize=address /Zi'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $def /I`"$out`" `"$PSScriptRoot/TuureDefeatTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Tuure defeat native regression failed'}
}
