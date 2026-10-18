$ErrorActionPreference='Stop'
# Reuse the existing native PE/MAP readers and their unchanged Grey Shoals
# checks, then the exception-directory bounds/activation verification.
. "$PSScriptRoot/Test-GreyShoalsRelease.ps1"
. "$PSScriptRoot/Test-VrektRelease.ps1"
$scriptsRelative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'scripts\\Release\\scripts.lib$'} | Select-Object -First 1
$scriptsLib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $scriptsRelative))
foreach($path in @('src/server/scripts/Legion/zone_stormheim.cpp','src/server/scripts/Legion/vrekt.h')){
 if((Get-Item "$repo/$path").LastWriteTimeUtc -gt (Get-Item $scriptsLib).LastWriteTimeUtc){throw "Final-link scripts predates $path"}
}
foreach($path in @('src/server/game/Entities/Creature/Creature.cpp','src/server/game/Entities/Unit/Unit.cpp')){
 if((Get-Item "$repo/$path").LastWriteTimeUtc -gt (Get-Item $lib).LastWriteTimeUtc){throw "Final-link core predates $path"}
}
function CountVirtualCalls([string]$method,[int]$slot){
 $range=Bounds $method;$at=Offset $range[0];$count=0
 for($i=0;$i -lt $range[1]-$range[0]-6;$i++){
  if($binary[$at+$i] -ne 0xff){continue};$mod=$binary[$at+$i+1]
  if($mod -ge 0x90 -and $mod -le 0x97 -and [BitConverter]::ToInt32($binary,$at+$i+2) -eq $slot){++$count}
  if($slot -lt 128 -and $mod -ge 0x50 -and $mod -le 0x57 -and $binary[$at+$i+2] -eq $slot){++$count}
 };$count
}
$table='??_7npc_vrekt_rotation@@6B@'
foreach($pair in @(@('?Update@Creature@@UEAAXI@Z','?UpdateLifecycle@npc_vrekt_rotation@@'),@('?IsNeverVisible@Creature@@','?CanBeVisibleFor@npc_vrekt_rotation@@'),@('?DealDamage@Unit@@','?DamageTakenAfterScaling@npc_vrekt_rotation@@'))){
 $slot=VirtualSlot $table $pair[1]
 if((CountVirtualCalls $pair[0] $slot) -lt 2){throw "Canonical $($pair[0]) lacks current and suspended policy dispatch"}
}
[void](VirtualSlot $table '?OnCharmed@CreatureAI@@')
foreach($name in @('UpdateAI','MoveInLineOfSight','EnterCombat','JustDied')){
 if(@(Calls "?$name@npc_vrekt_rotation@@") -notcontains (Resolve (Symbol "?$name@AggressorAI@@"))){throw "Vrekt wrapper does not call native AggressorAI::$name"}
}
$calls=@(Calls '?Sync@npc_vrekt_rotation@@')
foreach($name in @('?RemoveCharmAuras@Unit@@','?RemoveCharmedBy@Unit@@','?GetWorldQuest@QuestDataStoreMgr@@','?GetGameTime@GameTime@@')){
 if($calls -notcontains (Resolve (Symbol $name))){throw "Deployed Vrekt policy lacks $name"}
}
'PASS: canonical Vrekt vtable retains native CreatureAI charm swap; wrappers call AggressorAI, Sync has clock/rotation/native control cleanup, and core has both current/suspended lifecycle/visibility/final-damage dispatch. Actual final-link inputs verified; no client claim.'
