$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$exe="$repo/build-extractors/bin/Release/worldserver.exe"
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 $relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'scripts\\Release\\scripts.lib$'} | Select-Object -First 1
 if(!$relative){throw 'Missing actual scripts final-link input'}
 $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
 if((Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Canonical Release predates scripts library'}
 foreach($path in @('src/server/scripts/Legion/zone_azsuna.cpp','src/server/scripts/Legion/grey_shoals.h')){
  if((Get-Item "$repo/$path").LastWriteTimeUtc -gt (Get-Item $lib).LastWriteTimeUtc){throw "Release scripts predates source: $path"}
 }
 $map=Get-Content "$repo/build-extractors/bin/Release/worldserver.map" -Raw
 function Symbol([string]$name){
  $index=$map.IndexOf(' '+$name);if($index -lt 0){throw "Missing canonical symbol: $name"}
  $line=$map.Substring($index,$map.IndexOf("`n",$index)-$index)
  $m=[regex]::Match($line,'\s([0-9A-Fa-f]{16})\s+f')
  if(!$m.Success){throw "Missing canonical symbol address: $name"};[Convert]::ToInt64($m.Groups[1].Value,16)
 }
 $binary=[IO.File]::ReadAllBytes($exe);$pe=[BitConverter]::ToInt32($binary,0x3c);$optional=$pe+24
 $imageBase=[BitConverter]::ToInt64($binary,$optional+24);$sections=$optional+[BitConverter]::ToUInt16($binary,$pe+20)
 function Offset([long]$address){
  $relative=$address-$imageBase
  for($i=0;$i -lt [BitConverter]::ToUInt16($binary,$pe+6);$i++){
   $section=$sections+40*$i;$start=[BitConverter]::ToUInt32($binary,$section+12)
   $size=[Math]::Max([BitConverter]::ToUInt32($binary,$section+8),[BitConverter]::ToUInt32($binary,$section+16))
   if($relative -ge $start -and $relative -lt $start+$size){return [long]([BitConverter]::ToUInt32($binary,$section+20)+$relative-$start)}
  };throw 'Address outside canonical PE'
 }
 function Resolve([long]$address){
  for($i=0;$i -lt 10;$i++){$at=Offset $address;if($binary[$at] -ne 0xe9){return $address};$address+=5+[BitConverter]::ToInt32($binary,$at+1)}
  throw 'Unexpected incremental-link thunk cycle'
 }
 function Calls([string]$name){
  $address=Symbol $name;$index=$map.IndexOf(' '+$name);$after=$map.IndexOf("`n",$index)+1
  $tail=$map.Substring($after,[Math]::Min(8192,$map.Length-$after));$end=$address+4096
  foreach($m in [regex]::Matches($tail,'\s([0-9A-Fa-f]{16})\s+f')){$v=[Convert]::ToInt64($m.Groups[1].Value,16);if($v -gt $address){$end=[Math]::Min($end,$v)}}
  $at=Offset $address
  for($i=0;$i -lt $end-$address-4;$i++){
   if($binary[$at+$i] -in @(0xe8,0xe9)){try{Resolve ($address+$i+5+[BitConverter]::ToInt32($binary,$at+$i+1))}catch{}}
  }
 }

$calls=@(Calls '?OnGossipHello@go_grey_shoals_supplies@@')
foreach($name in @('?GetWorldQuest@QuestDataStoreMgr@@','?GetGameTime@GameTime@@','?GetQuestStatus@Player@@')){
 if($calls -notcontains (Resolve (Symbol $name))){throw "Canonical supply handler lacks $name"}
}
if($calls -contains (Resolve (Symbol '?KilledMonsterCredit@Player@@'))){throw 'Canonical supply handler retains personal shortcut instead of native229972 group credit'}
$coreRelative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'game\\Release\\game.lib$'} | Select-Object -First 1
if(!$coreRelative){throw 'Missing actual game final-link input'}
$coreLib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $coreRelative))
if((Get-Item $coreLib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Canonical Release predates core library'}
foreach($path in @('src/server/game/AI/CreatureAI.h','src/server/game/AI/CoreAI/GameObjectAI.h','src/server/game/Entities/Creature/Creature.cpp','src/server/game/Entities/GameObject/GameObject.cpp')){
 if((Get-Item "$repo/$path").LastWriteTimeUtc -gt (Get-Item $coreLib).LastWriteTimeUtc){throw "Core library predates $path"}
}
function DataSymbol([string]$name){
 $index=$map.IndexOf(' '+$name);if($index -lt 0){throw "Missing canonical vtable $name"}
 $line=$map.Substring($index,$map.IndexOf("`n",$index)-$index)
 $m=[regex]::Match($line,'\s([0-9A-Fa-f]{16})\s+')
 if(!$m.Success){throw 'Missing canonical vtable address'};[Convert]::ToInt64($m.Groups[1].Value,16)
}
function VirtualSlot([string]$table,[string]$method){
 $at=Offset (DataSymbol $table);$target=Resolve (Symbol $method)
 for($slot=0;$slot -lt 160;$slot++){
  $value=[BitConverter]::ToInt64($binary,$at+8*$slot)
  try{if((Resolve $value) -eq $target){return $slot*8}}catch{}
 };throw "Missing deployed vtable slot $method"
}
function VirtualCall([string]$method,[int]$slot){
 $address=Resolve (Symbol $method);$at=Offset $address
 $index=$map.IndexOf(' '+$method);$after=$map.IndexOf("`n",$index)+1;$end=$address+32768
 foreach($m in [regex]::Matches($map.Substring($after,[Math]::Min(65536,$map.Length-$after)),'\s([0-9A-Fa-f]{16})\s+f')){
  $v=[Convert]::ToInt64($m.Groups[1].Value,16);if($v -gt $address){$end=[Math]::Min($end,$v)}
 }
 for($i=0;$i -lt $end-$address-6;$i++){
  if($binary[$at+$i] -ne 0xff){continue};$mod=$binary[$at+$i+1]
  if($mod -ge 0x90 -and $mod -le 0x97 -and [BitConverter]::ToInt32($binary,$at+$i+2) -eq $slot){return}
  if($slot -lt 128 -and $mod -ge 0x50 -and $mod -le 0x57 -and $binary[$at+$i+2] -eq $slot){return}
 };throw "Deployed $method lacks visibility/lifecycle virtual call at slot$slot"
}
$lifecycleSlot=VirtualSlot '??_7npc_grey_shoals_rotation@@6B@' '?UpdateLifecycle@npc_grey_shoals_rotation@@'
$creatureSlot=VirtualSlot '??_7npc_grey_shoals_rotation@@6B@' '?CanBeVisibleFor@npc_grey_shoals_rotation@@'
$objectSlot=VirtualSlot '??_7go_grey_shoals_rotation@@6B@' '?CanBeVisibleFor@go_grey_shoals_rotation@@'
VirtualCall '?Update@Creature@@UEAAXI@Z' $lifecycleSlot
VirtualCall '?IsNeverVisible@Creature@@' $creatureSlot
VirtualCall '?IsNeverVisible@GameObject@@' $objectSlot
foreach($method in @('?CanBeVisibleFor@npc_grey_shoals_rotation@@','?CanBeVisibleFor@go_grey_shoals_rotation@@')){
 $visibilityCalls=@(Calls $method)
 foreach($name in @('?GetWorldQuest@QuestDataStoreMgr@@','?GetGameTime@GameTime@@')){
  if($visibilityCalls -notcontains (Resolve (Symbol $name))){throw "Deployed $method lacks $name"}
 }
}
'PASS: actual canonical game/scripts final-link inputs, native lifecycle/visibility virtual slots and both deployed rotation filters.'
$controlCalls=@(Calls '?Sync@npc_grey_shoals_rotation@@')
foreach($name in @('?RemoveCharmAuras@Unit@@','?RemoveCharmedBy@Unit@@')){
 if($controlCalls -notcontains (Resolve (Symbol $name))){throw "Canonical Grey Shoals rotation lacks native control release: $name"}
}
'PASS: canonical scoped rotation invokes native aura and direct-charm cleanup.'
'PASS: actual final-link scripts library/source freshness and deployed supply handler rotation, time, quest status and removal of personal credit shortcut. Client remains pending.'
