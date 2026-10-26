param([switch]$Baseline,[switch]$AddressSanitizer,[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/devourer-capture-sequence"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/$(if($Baseline){'baseline'}else{'current'})" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $path='src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp'
 $current=Get-Content "$repo/$path" -Raw
 $old=(& git -C $repo show "e4e4fe2f:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned capture baseline unavailable'}
 $class=Block $current 'class npc_akaari_shadowgore :';$oldClass=Block $old 'class npc_akaari_shadowgore :'
 if(($current.Replace($class,'')-replace '\s','') -cne ($old.Replace($oldClass,'')-replace '\s','')){throw 'Unexpected change to another Devourer actor/pickup'}
 [IO.File]::WriteAllText("$out/Actor.inc",(Block $(if($Baseline){$oldClass}else{$class}) 'struct npc_akaari_shadowgoreAI :')+';')
 $eh=Get-Content "$repo/src/server/game/AI/EventMap.h" -Raw;$es=Get-Content "$repo/src/server/game/AI/EventMap.cpp" -Raw
 [IO.File]::WriteAllText("$out/EventMapClass.inc",(Block $eh 'enum c_events')+';'+(Block $eh 'class TC_GAME_API EventMap').Replace('TC_GAME_API ','')+';')
 [IO.File]::WriteAllText("$out/EventMapMethods.inc",((@('void EventMap::Reset(','void EventMap::ScheduleEvent(uint32 eventId, uint32 time','uint32 EventMap::ExecuteEvent(','void EventMap::CancelEvent(')|ForEach-Object {Block $es $_}) -join "`n"))
 $fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
 [IO.File]::WriteAllText("$out/FunctionProcessor.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
 $accessor=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.h" -Raw;$methods=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.cpp" -Raw
 [IO.File]::WriteAllText("$out/Find.inc",(Block $accessor 'static T* Find(ObjectGuid guid)'))
 [IO.File]::WriteAllText("$out/Lookup.inc",(Block $accessor 'static Player* GetObjectInWorld(ObjectGuid guid, Player*')+(Block $accessor 'template<class T> static T* GetObjectInWorld(ObjectGuid guid, T*')+(Block $accessor 'template<class T> static T* GetObjectInMap(')+'static Player* GetPlayer(WorldObject const&,ObjectGuid);static Creature* GetCreature(WorldObject const&,ObjectGuid);')
 [IO.File]::WriteAllText("$out/LookupMethods.inc",(Block $methods 'Player* ObjectAccessor::GetPlayer(')+(Block $methods 'Creature* ObjectAccessor::GetCreature('))
 [IO.File]::WriteAllText("$out/OwnerGuid.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'ObjectGuid GetCharmerOrOwnerGUID() const'))
 [IO.File]::WriteAllText("$out/Owner.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw) 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'))
 [IO.File]::WriteAllText("$out/PendingTeleport.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.h" -Raw) 'bool HasPendingTeleport() const'))
 $playerSource=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
 [IO.File]::WriteAllText("$out/QuestDelay.inc",(Block $playerSource 'void Player::AddQuestDelayedEvent('))
 if($playerSource -notmatch 'status_q.ScriptLifetime.reset\(\);' -or (Block $playerSource 'void Player::SetQuestStatus(') -notmatch 'status == QUEST_STATUS_NONE\)\s+q_status.ScriptLifetime.reset\(\);'){throw 'Native quest lease no longer expires on accept/remove'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $def=if($Baseline){'/DBASELINE'}else{''};if($AddressSanitizer){$def+=' /fsanitize=address /Zi'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $def /I`"$out`" `"$PSScriptRoot/DevourerCaptureSequenceTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Capture sequence regression failed'}
 if($VerifyNativeCatalog){$prior=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile DevourerCaptureCatalog.cpp -ProbeName DevourerCaptureCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Capture native spell catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$prior};$rows=Get-Content "$out/native.tsv";foreach($row in @("DURATION`t209238`t20000","DURATION`t209264`t7000","EFFECT`t209264`t0`t6`t260`t963`t0`t92`t0","DURATION`t211684`t-1","EFFECT`t211686`t0`t6`t67`t0`t0`t1`t0")){if($rows -notcontains $row){throw "Native capture metadata changed: $row"}}}
}
