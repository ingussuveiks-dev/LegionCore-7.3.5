$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$exe="$repo/build-extractors/bin/Release/worldserver.exe"
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 $relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'scripts\\Release\\scripts.lib$'} | Select-Object -First 1
 if(!$relative){throw 'Missing actual scripts final-link input'}
 $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
 if((Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Canonical Release predates scripts library'}
 foreach($path in @('src/server/scripts/Legion/zone_highmountain.cpp','src/server/scripts/Legion/rockaway_coast.h')){
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

$finish=@(Calls '?Finish@npc_rockaway_vision@@')
foreach($name in @('?KilledMonsterCredit@Player@@QEAAXIVObjectGuid@@@Z','?AutoStoreLoot@Player@@QEAA_NEEIAEBVLootStore@@_NI@Z')){
 if($finish -notcontains (Resolve (Symbol $name))){throw "Deployed Rockaway Finish lacks $name"}
}
if($finish -notcontains (Resolve (Symbol '?DespawnOrUnsummon@Creature@@'))){throw 'Deployed Finish lacks retained corpse lifetime scheduling'}
$update=@(Calls '?UpdateAI@npc_rockaway_vision@@')
if($update -notcontains (Resolve (Symbol '?Finish@npc_rockaway_vision@@'))){throw 'Deployed UpdateAI lacks final completion callback'}
if($update -contains (Resolve (Symbol '?AddAura@Unit@@QEAAPEAVAura@@I'))){throw 'Deployed Vision retains unsupported Mage Tower aura application'}
$death=@(Calls '?JustDied@npc_rockaway_ritualist@@')
if($death -notcontains (Resolve (Symbol '?StopChannel@npc_rockaway_ritualist@@'))){throw 'Deployed ritual death lacks channel cleanup'}
$stop=@(Calls '?StopChannel@npc_rockaway_ritualist@@')
foreach($name in @('?CastStop@Unit@@','?RemoveAurasDueToSpell@Unit@@')){if($stop -notcontains (Resolve (Symbol $name))){throw "Deployed channel cleanup lacks $name"}}
$ritual=@(Calls '?UpdateAI@npc_rockaway_ritualist@@')
foreach($name in @('?ChannelVision@Rockaway@@','?StopChannel@npc_rockaway_ritualist@@')){if($ritual -notcontains (Resolve (Symbol $name))){throw "Deployed ritual channel lacks $name"}}
# GetChannelSpellId is inlined in this Release. Check the selected spell ID
# in the actual UpdateAI body as well as the non-inlined admission/cleanup calls.
$address=Symbol '?UpdateAI@npc_rockaway_ritualist@@';$at=Offset $address
$index=$map.IndexOf(' ?UpdateAI@npc_rockaway_ritualist@@');$after=$map.IndexOf("`n",$index)+1;$end=$address+4096
foreach($m in [regex]::Matches($map.Substring($after,[Math]::Min(8192,$map.Length-$after)),'\s([0-9A-Fa-f]{16})\s+f')){$v=[Convert]::ToInt64($m.Groups[1].Value,16);if($v -gt $address){$end=[Math]::Min($end,$v)}}
$channelId=$false
for($i=0;$i -lt $end-$address-3;$i++){if([BitConverter]::ToUInt32($binary,$at+$i) -eq 231433){$channelId=$true;break}}
if(!$channelId){throw 'Deployed ritual UpdateAI lacks231433'}
$target=@(Calls '?Select@spell_rockaway_channel@@')
if($target -notcontains (Resolve (Symbol '?ChannelVision@Rockaway@@'))){throw 'Deployed target selector lacks exact controller admission'}
$damage=@(Calls '?DamageTakenAfterScaling@npc_rockaway_vision@@')
if($damage -notcontains (Resolve (Symbol '?GetHealth@Unit@@QEBA_KPEAV1@@Z'))){throw 'Deployed Vision lacks target-relative scaled damage clamp'}
'PASS: canonical Release final-link scripts library, exact-owner channel hook, retry/death cleanup, no Vision Mage Tower aura, completion/credit/loot calls and scaled health clamp.'
