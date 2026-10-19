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
# Bound each x64 function with the executable's own exception directory,
# including when the public MAP symbol resolves through an incremental thunk.
$pdata=Offset ($imageBase+[BitConverter]::ToUInt32($binary,$optional+112+3*8))
$entries=[BitConverter]::ToUInt32($binary,$optional+112+3*8+4)/12
function UnwindRoot([long]$begin,[long]$unwind){
 for($depth=0;$depth -lt 16;$depth++){
  $at=Offset ($imageBase+$unwind)
  if(!($binary[$at] -band 0x20)){return $begin} # UNW_FLAG_CHAININFO (flags occupy the high5 bits).
  $chain=$at+4+4*[int][Math]::Ceiling($binary[$at+2]/2.0)
  $begin=[BitConverter]::ToUInt32($binary,$chain)
  $unwind=[BitConverter]::ToUInt32($binary,$chain+8)
 };throw 'Unexpected chained unwind cycle'
}
function Bounds([string]$name){
 $address=Resolve (Symbol $name);$rva=$address-$imageBase;$low=0;$high=[int]$entries-1
 while($low -le $high){$mid=[int][Math]::Floor(($low+$high)/2);$at=$pdata+12*$mid
  $start=[BitConverter]::ToUInt32($binary,$at);$end=[BitConverter]::ToUInt32($binary,$at+4)
  if($rva -lt $start){$high=$mid-1}elseif($rva -ge $end){$low=$mid+1}else{
   $rootBegin=UnwindRoot $start ([BitConverter]::ToUInt32($binary,$at+8))
   # MSVC shrink wrapping splits one contiguous body into several runtime
   # function records. Include only adjacent fragments chained to this root,
   # never arbitrary bytes up to the next public MAP symbol.
   for($next=$mid+1;$next -lt $entries;$next++){
    $tail=$pdata+12*$next;$tailBegin=[BitConverter]::ToUInt32($binary,$tail)
    if($tailBegin -ne $end -or (UnwindRoot $tailBegin ([BitConverter]::ToUInt32($binary,$tail+8))) -ne $rootBegin){break}
    $end=[BitConverter]::ToUInt32($binary,$tail+4)
   }
   return @($address,[long]($imageBase+$end))
  }
 };throw "Missing x64 function range $name"
}
function Calls([string]$name){
 $range=Bounds $name;$at=Offset $range[0]
 for($i=0;$i -lt $range[1]-$range[0]-4;$i++){
  if($binary[$at+$i] -in @(0xe8,0xe9)){try{Resolve ($range[0]+$i+5+[BitConverter]::ToInt32($binary,$at+$i+1))}catch{}}
 }
}

$finish=@(Calls '?Finish@npc_rockaway_vision@@')
$eligible=@(Calls '?Eligible@Rockaway@@')
if($eligible -notcontains (Resolve (Symbol '?CanRewardWorldQuest@Player@@'))){throw 'Deployed Rockaway recipient eligibility lacks native reward admission'}
if($finish -notcontains (Resolve (Symbol '?Eligible@Rockaway@@')) -and
   $finish -notcontains (Resolve (Symbol '?CanRewardWorldQuest@Player@@'))){throw 'Deployed Finish bypasses guarded recipient eligibility'}
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
$range=Bounds '?SummonedCreatureDies@npc_rockaway_vision@@'
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$dump=Get-ChildItem "$vs/VC/Tools/MSVC/*/bin/Hostx64/x64/dumpbin.exe" | Select-Object -Last 1
if(!$dump){throw 'Native MSVC disassembler unavailable'}
$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
$listing=(& $dump.FullName /DISASM:NOBYTES $argument $exe) -join "`n"
if($LASTEXITCODE -or $listing -notmatch '\bmov\s+dword ptr \[[^\]]+\],1B58h'){
 throw 'Canonical final-death callback lacks its7000ms countdown assignment'
}
$output="$repo/build-extractors/runtime-test/rockaway-audio"
New-Item -ItemType Directory -Force $output | Out-Null
$listing | Set-Content "$output/finale-disassembly.txt"
'PASS: canonical Release final-link scripts library, exception-bounded7000ms final pause, native recipient reward admission, exact-owner channel hook, retry/death cleanup, no Vision Mage Tower aura, completion/credit/loot calls and scaled health clamp.'
