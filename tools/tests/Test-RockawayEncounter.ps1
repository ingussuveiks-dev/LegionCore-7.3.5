$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/rockaway-encounter"
New-Item -ItemType Directory -Force -Path $output | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/server/scripts/Legion','src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$source=(Get-Content "$repo/src/server/scripts/Legion/rockaway_coast.h" -Raw).Replace("`r",'')
$boundary=$source.IndexOf('class spell_rockaway_channel')
if($boundary -lt 0){throw 'Missing production encounter boundary'}
[IO.File]::WriteAllText("$output/RockawayActors.inc",$source.Substring(0,$boundary)+"`n#endif`n")
$a=$source.IndexOf('    void OnRemove(', $boundary);$b=$source.IndexOf('    void Register()', $a)
if($a -lt 0 -or $b -le $a){throw 'Missing actual taint removal callback'}
[IO.File]::WriteAllText("$output/RockawayTaint.inc",$source.Substring($a,$b-$a))
$a=$source.IndexOf('class spell_rockaway_channel');$b=$source.IndexOf('class spell_rockaway_taint',$a)
[IO.File]::WriteAllText("$output/RockawayChannel.inc",$source.Substring($a,$b-$a))
$spell=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
foreach($part in @(@('void Spell::SelectImplicitNearbyTargets(', 'void Spell::SelectImplicitTargetsFromThreadList(', 'RockawayNearby.inc'),@('void Spell::CallScriptObjectTargetSelectHandlers(', 'void Spell::CallScriptObjectJumpTargetHandlers(', 'RockawayTargetHooks.inc'),@('void Spell::SendChannelUpdate(', 'void Spell::SendResurrectRequest(', 'RockawayChannelFields.inc'))){
 $a=$spell.IndexOf($part[0]);$b=$spell.IndexOf($part[1],$a)
 if($a -lt 0 -or $b -le $a){throw 'Missing actual native nearby-target path'}
 [IO.File]::WriteAllText("$output/$($part[2])",$spell.Substring($a,$b-$a))
}
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$a=$unit.IndexOf('        // Call creature just died function');$b=$unit.IndexOf('        // Call instance script.', $a)
if($a -lt 0 -or $b -le $a){throw 'Missing native death/summoner dispatcher'}
[IO.File]::WriteAllText("$output/RockawayDeath.inc",$unit.Substring($a,$b-$a))
$a=$unit.IndexOf('void Unit::SetCurrentCastedSpell(');$b=$unit.IndexOf('void Unit::InterruptSpell(', $a)
if($a -lt 0 -or $b -le $a){throw 'Missing native spell-container/casting-state dispatcher'}
[IO.File]::WriteAllText("$output/RockawayCurrentSpell.inc",$unit.Substring($a,$b-$a))
$event=Get-Content "$repo/src/server/game/AI/EventMap.cpp" -Raw
$functions=@(@('void EventMap::Reset()','void EventMap::SetPhase('),@('void EventMap::ScheduleEvent(uint32 eventId, uint32 time','void EventMap::RescheduleEvent('),@('uint32 EventMap::ExecuteEvent()','void EventMap::DelayEvent('))
$export=''
foreach($part in $functions){$a=$event.IndexOf($part[0]);$b=$event.IndexOf($part[1],$a);if($a -lt 0 -or $b -le $a){throw 'Missing native EventMap method'};$export+=$event.Substring($a,$b-$a)}
[IO.File]::WriteAllText("$output/RockawayEvents.inc",$export)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='/I"'+$output+'"';$compilerArgs+='"'+$repo+'/tools/tests/RockawayEncounterTest.cpp"'
$compilerArgs+='/Fe"'+$output+'/RockawayEncounterTest.exe"';$compilerArgs+='/Fo"'+$output+'/RockawayEncounterTest.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-ground.cmd",$cmd)
& "$output/build-ground.cmd"
if($LASTEXITCODE){throw 'Rockaway native catalog build failed'}
$lines=@(& "$output/RockawayEncounterTest.exe")
$lines | Set-Content "$output/results.txt";$lines
if($LASTEXITCODE){throw 'Production Rockaway encounter regression failed'}
$old=(& git -C $repo show 'ec4f7a3b:src/server/scripts/Legion/rockaway_coast.h') -join "`n"
if($LASTEXITCODE){throw 'Missing pinned pre-fix Rockaway AI'}
$boundary=$old.IndexOf('class spell_rockaway_taint')
if($boundary -lt 0){throw 'Missing pinned old actor boundary'}
[IO.File]::WriteAllText("$output/RockawayOldActors.inc",$old.Substring(0,$boundary)+"`n#endif`n")
$oldArgs=@('/DROCKAWAY_BASELINE')+@($compilerArgs | ForEach-Object {$_.Replace('RockawayEncounterTest.exe','RockawayOldEncounterTest.exe').Replace('RockawayEncounterTest.obj','RockawayOldEncounterTest.obj')})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($oldArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-old.cmd",$cmd)
& "$output/build-old.cmd"
if($LASTEXITCODE){throw 'Pinned channel regression build failed'}
& "$output/RockawayOldEncounterTest.exe"
if($LASTEXITCODE){throw 'Pinned channel failure was not reproduced'}
$timing=(& git -C $repo show '516fc1f1:src/server/scripts/Legion/rockaway_coast.h') -join "`n"
if($LASTEXITCODE){throw 'Missing pinned pre-audio-timing AI'}
$boundary=$timing.IndexOf('class spell_rockaway_channel')
if($boundary -lt 0){throw 'Missing pinned timing actor boundary'}
[IO.File]::WriteAllText("$output/RockawayTimingActors.inc",$timing.Substring(0,$boundary)+"`n#endif`n")
$timingArgs=@('/DROCKAWAY_TIMING_BASELINE')+@($compilerArgs | ForEach-Object {$_.Replace('RockawayEncounterTest.exe','RockawayTimingTest.exe').Replace('RockawayEncounterTest.obj','RockawayTimingTest.obj')})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($timingArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-timing.cmd",$cmd);& "$output/build-timing.cmd"
if($LASTEXITCODE){throw 'Pinned finale timing build failed'}
& "$output/RockawayTimingTest.exe"
if($LASTEXITCODE){throw 'Pinned early-credit timing was not reproduced'}
